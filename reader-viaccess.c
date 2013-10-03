#include "globals.h"
#ifdef READER_VIACCESS
#include "oscam-aes.h"
#include "oscam-time.h"
#include "oscam-emm.h"
#include "reader-common.h"

struct geo_cache
{
    uint32_t        provid;
    uint8_t         geo[256];
    uint8_t         geo_len;
    int32_t         number_ecm;
};

struct viaccess_data
{
    struct geo_cache    last_geo;
    uint8_t             availkeys[CS_MAXPROV][16];
};

struct via_date
{
    uint16_t day_s   : 5;
    uint16_t month_s : 4;
    uint16_t year_s  : 7;

    uint16_t day_e   : 5;
    uint16_t month_e : 4;
    uint16_t year_e  : 7;
};

static void parse_via_date(const uchar *buf, struct via_date *vd, int32_t fend)
{
    uint16_t date;

    date = (buf[0] << 8) | buf[1];
    vd->day_s   = date & 0x1f;
    vd->month_s = (date >> 5) & 0x0f;
    vd->year_s  = (date >> 9) & 0x7f;

    if ( fend )
    {
        date = (buf[2] << 8) | buf[3];
        vd->day_e   = date & 0x1f;
        vd->month_e = (date >> 5) & 0x0f;
        vd->year_e  = (date >> 9) & 0x7f;
    }
}

//static void get_via_data(const uchar *b, int32_t l, time_t *start_t, time_t *end_t, uchar *cls)
//{
//  int32_t i, j;
//  struct via_date vd;
//  struct tm tm;
//  memset(&vd, 0, sizeof(struct via_date));
//
//  // b -> via date (4 bytes)
//  b+=4;
//  l-=4;
//
//  j=l-1;
//  for (; j>=0; j--)
//      for (i=0; i<8; i++)
//          if (b[j] & (1 << (i&7)))
//          {
//              parse_via_date(b-4, &vd, 1);
//              *cls=(l-(j+1))*8+i;
//          }
//
//  memset(&tm, 0, sizeof(struct tm));
//  tm.tm_year = vd.year_s + 80;    //via year starts in 1980, tm_year starts in 1900
//  tm.tm_mon = vd.month_s - 1; // january is 0 in tm_mon
//  tm.tm_mday = vd.day_s;
//  *start_t = mktime(&tm);
//
//  tm.tm_year = vd.year_e + 80;
//  tm.tm_mon = vd.month_e - 1;
//  tm.tm_mday = vd.day_e;
//  *end_t = mktime(&tm);
//
//}

static void show_class(struct s_reader *reader, const char *p, uint32_t provid, const uchar *b, int32_t l)
{
    int32_t i, j;

    // b -> via date (4 bytes)
    b += 4;
    l -= 4;

    j = l - 1;
    for (; j >= 0; j--)
        for (i = 0; i < 8; i++)
            if (b[j] & (1 << (i & 7)))
            {
                uchar cls;
                struct via_date vd;
                parse_via_date(b - 4, &vd, 1);
                cls = (l - (j + 1)) * 8 + i;
                if (p)
                    rdr_log(reader, "%sclass: %02X, expiry date: %04d/%02d/%02d - %04d/%02d/%02d", p, cls,
                            vd.year_s + 1980, vd.month_s, vd.day_s,
                            vd.year_e + 1980, vd.month_e, vd.day_e);
                else
                {
                    rdr_log(reader, "class: %02X, expiry date: %04d/%02d/%02d - %04d/%02d/%02d", cls,
                            vd.year_s + 1980, vd.month_s, vd.day_s,
                            vd.year_e + 1980, vd.month_e, vd.day_e);

                    time_t start_t, end_t;
                    struct tm tm;
                    //convert time:
                    memset(&tm, 0, sizeof(tm));
                    tm.tm_year = vd.year_s + 80; //via year starts in 1980, tm_year starts in 1900
                    tm.tm_mon = vd.month_s - 1; // january is 0 in tm_mon
                    tm.tm_mday = vd.day_s;
                    start_t = cs_timegm(&tm);

                    tm.tm_year = vd.year_e + 80; //via year starts in 1980, tm_year starts in 1900
                    tm.tm_mon = vd.month_e - 1; // january is 0 in tm_mon
                    tm.tm_mday = vd.day_e;
                    end_t = cs_timegm(&tm);

                    cs_add_entitlement(reader, reader->caid, provid, cls, cls, start_t, end_t, 5);
                }
            }
}

static void show_subs(struct s_reader *reader, const uchar *emm)
{
    // emm -> A9, A6, B6

    switch ( emm[0] )
    {
    case 0xA9:
        show_class(reader, "nano A9: ", 0, emm + 2, emm[1]);
        break;
    /*
    {
    int32_t i, j, byts;
    const uchar *oemm;

    oemm = emm;
    byts = emm[1]-4;
    emm+=6;

    j=byts-1;
    for( ; j>=0; j-- )
    for( i=0; i<8; i++ )
    if( emm[j] & (1 << (i&7)) )
    {
    uchar cls;
    struct via_date vd;
    parse_via_date(emm-4, &vd, 1);
    cls=(byts-(j+1))*8+i;
    rdr_log(reader, "%sclass %02X: expiry date: %02d/%02d/%04d - %02d/%02d/%04d",
    fnano?"nano A9: ":"", cls,
    vd.day_s, vd.month_s, vd.year_s+1980,
    vd.day_e, vd.month_e, vd.year_e+1980);
    }
    break;
    }
    */
    case 0xA6:
    {
        char szGeo[256];

        memset(szGeo, 0, 256);
        strncpy(szGeo, (char *)emm + 2, emm[1]);
        rdr_log(reader, "nano A6: geo %s", szGeo);
        break;
    }
    case 0xB6:
    {
        uchar m; // modexp
        struct via_date vd;

        m = emm[emm[1] + 1];
        parse_via_date(emm + 2, &vd, 0);
        rdr_log(reader, "nano B6: modexp %d%d%d%d%d%d: %02d/%02d/%04d", (m & 0x20) ? 1 : 0,
                (m & 0x10) ? 1 : 0, (m & 0x08) ? 1 : 0, (m & 0x04) ? 1 : 0, (m & 0x02) ? 1 : 0, (m & 0x01) ? 1 : 0,
                vd.day_s, vd.month_s, vd.year_s + 1980);
        break;
    }
    }
}

static int32_t chk_prov(struct s_reader *reader, uchar *id, uchar keynr)
{
    struct viaccess_data *csystem_data = reader->csystem_data;
    int32_t i, j, rc;
    for (rc = i = 0; (!rc) && (i < reader->nprov); i++)
        if (!memcmp(&reader->prid[i][1], id, 3))
            for (j = 0; (!rc) && (j < 16); j++)
                if (csystem_data->availkeys[i][j] == keynr)
                    rc = 1;
    return (rc);
}

static int32_t unlock_parental(struct s_reader *reader)
{
    /* disabling parental lock. assuming pin "0000" if no pin code is provided in the config */

    static const uchar inDPL[] = {0xca, 0x24, 0x02, 0x00, 0x09};
    uchar cmDPL[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F};
    def_resp;

    if (strcmp(reader->pincode, "none"))
    {
        rdr_log(reader, "Using PIN %s", reader->pincode);
        // the pin need to be coded in bcd, so we need to convert from ascii to bcd, so '1234' -> 0x12 0x34
        cmDPL[6] = ((reader->pincode[0] - 0x30) << 4) | ((reader->pincode[1] - 0x30) & 0x0f);
        cmDPL[7] = ((reader->pincode[2] - 0x30) << 4) | ((reader->pincode[3] - 0x30) & 0x0f);
    }
    else
    {
        rdr_log(reader, "Using PIN 0000!");
    }
    write_cmd(inDPL, cmDPL);
    if ( !(cta_res[cta_lr - 2] == 0x90 && cta_res[cta_lr - 1] == 0) )
    {
        if (strcmp(reader->pincode, "none"))
        {
            rdr_log(reader, "Can't disable parental lock. Wrong PIN? OSCam used %s!", reader->pincode);
        }
        else
        {
            rdr_log(reader, "Can't disable parental lock. Wrong PIN? OSCam used 0000!");
        }
    }
    else
        rdr_log(reader, "Parental lock disabled");

    return 0;
}

int32_t hdSurEncBasicCrypt(int32_t Value, int32_t XorVal)
{
    int32_t i = (Value << 13) - Value + 0x1B59;
    i = (i * Value) + 0x07CF;
    return (i ^ XorVal);
}

int32_t hdSurEncCryptLookup(uint8_t Value, uint8_t AddrInd)
{
    static const uint8_t lookup[] =
    {
        0x94, 0xB2, 0xA9, 0x79, 0xC4, 0xC7, 0x0D, 0x36, 0x6F, 0x24, 0x11, 0xD1, 0xDB, 0x59, 0xD2, 0xA5,
        0xE1, 0x00, 0xD4, 0x97, 0xA3, 0x2B, 0x11, 0xFA, 0x5F, 0xF1, 0xC1, 0x44, 0xBF, 0x9B, 0x5A, 0xC8,
        0xF1, 0xE1, 0x99, 0x82, 0x0E, 0xB2, 0x01, 0x09, 0x0C, 0xC8, 0xB3, 0x3B, 0xD1, 0x80, 0x50, 0xE8,
        0xF5, 0x52, 0x4C, 0xE6, 0x82, 0xAC, 0x58, 0x40, 0xD4, 0x71, 0x87, 0x52, 0x06, 0xEA, 0xA6, 0x27,
        0xB7, 0xFE, 0x6C, 0x49, 0x47, 0x3B, 0x70, 0x6C, 0xEB, 0xCD, 0xC5, 0x0B, 0x8C, 0x31, 0x29, 0x42,
        0x4E, 0x10, 0x2B, 0x2D, 0x46, 0xEC, 0x39, 0xA3, 0x90, 0x4B, 0x25, 0x60, 0x9C, 0x62, 0xD4, 0x20,
        0xF6, 0x16, 0xA8, 0x9C, 0xE4, 0x20, 0xED, 0xC7, 0xBA, 0x5E, 0xB6, 0x4E, 0x03, 0x15, 0xA6, 0xF6,
        0x23, 0x98, 0x32, 0xC0, 0xAE, 0xA3, 0xFD, 0xD3, 0x7F, 0xF8, 0xED, 0xF0, 0x29, 0x29, 0x12, 0xB3,
        0xB7, 0x58, 0xAD, 0xA2, 0x58, 0x2C, 0x70, 0x1B, 0xA4, 0x25, 0xE8, 0xA5, 0x43, 0xF1, 0xB9, 0x8F,
        0x1E, 0x3B, 0x10, 0xDF, 0x52, 0xFE, 0x58, 0x29, 0xAD, 0x3F, 0x99, 0x4D, 0xDF, 0xD2, 0x08, 0x06,
        0xA1, 0x1C, 0x66, 0x29, 0x26, 0x80, 0x52, 0x8A, 0x5A, 0x73, 0xE7, 0xDF, 0xC1, 0xC4, 0x47, 0x82,
        0xAB, 0x5C, 0x32, 0xAE, 0x96, 0x04, 0x2B, 0xC3, 0x2D, 0x5A, 0xD2, 0xB0, 0x64, 0x88, 0x97, 0xBF,
        0x7E, 0x99, 0x60, 0xCC, 0x63, 0x76, 0x66, 0xE9, 0x9A, 0x3D, 0xBB, 0xF7, 0x7F, 0xE4, 0x7C, 0x3F,
        0xB8, 0x4D, 0x10, 0x8D, 0x2A, 0xEA, 0x3C, 0xD3, 0x03, 0x74, 0xE6, 0x46, 0xC0, 0x29, 0xAE, 0xB0,
        0x79, 0xBE, 0xCB, 0x18, 0x34, 0xBE, 0x5A, 0xE9, 0x19, 0x8F, 0xA3, 0x8F, 0xD6, 0x6A, 0x6C, 0x88,
        0x1E, 0x21, 0x08, 0x15, 0xC4, 0xE7, 0xE6, 0xBA, 0x97, 0x9C, 0x4F, 0x89, 0x9F, 0x1A, 0x67, 0x4F,
        0xC0, 0xD5, 0x72, 0x51, 0x16, 0xB4, 0xD3, 0x8A, 0x1F, 0xE3, 0x92, 0x02, 0x7F, 0x59, 0x56, 0x8F,
        0x07, 0x8D, 0xC1, 0xC2, 0x42, 0x69, 0x3C, 0xA6, 0xBF, 0x3D, 0xDF, 0x0D, 0xAA, 0x4F, 0x7E, 0x80,
        0x07, 0x11, 0xE2, 0x94, 0x19, 0x9B, 0x16, 0x26, 0x1A, 0x46, 0x09, 0x0D, 0xB5, 0xB8, 0x8E, 0x01,
        0x9C, 0xFE, 0x09, 0xB3, 0x60, 0xC2, 0xAE, 0x50, 0x3C, 0x68, 0x75, 0x4A, 0x57, 0xD8, 0x4F, 0xD7,
        0xA2, 0x76, 0x2C, 0xC1, 0xA2, 0x23, 0xBC, 0x54, 0x2A, 0xDD, 0xF3, 0xDD, 0xA7, 0x34, 0xF7, 0x5C,
        0xF4, 0x86, 0x23, 0x48, 0x7C, 0x3F, 0x05, 0x40, 0x0E, 0xB0, 0xE5, 0xEB, 0x3E, 0xDF, 0x6A, 0x83,
        0x65, 0xA0, 0xB2, 0x06, 0xD1, 0x40, 0x79, 0x0D, 0xDE, 0x95, 0x84, 0x96, 0x87, 0x6F, 0xCE, 0x48,
        0x24, 0x13, 0x0B, 0xF5, 0xC7, 0xF5, 0xA8, 0x7F, 0x2E, 0xC7, 0xE1, 0xBA, 0xAE, 0x2B, 0xF7, 0xF0,
        0x8E, 0xF7, 0x54, 0x0B, 0xF0, 0xD2, 0x41, 0x81, 0x68, 0x3B, 0x1E, 0x35, 0xAB, 0xD9, 0x2B, 0x46,
        0x57, 0xE8, 0x53, 0xDF, 0xDE, 0x10, 0xEF, 0xCB, 0x4C, 0xE0, 0x52, 0x18, 0x2C, 0x4E, 0xB9, 0x20,
        0xE9, 0x7E, 0x85, 0xDF, 0x75, 0x32, 0xE6, 0x10, 0xE9, 0x9C, 0x7B, 0x2E, 0x4C, 0xDA, 0x46, 0xE6,
        0xCC, 0x77, 0x36, 0x1D, 0x4A, 0x15, 0xF5, 0x32, 0x18, 0x6B, 0x7E, 0xAA, 0xCC, 0x97, 0xCC, 0xD1,
        0x2F, 0xE5, 0x58, 0x03, 0x35, 0x35, 0x3D, 0xA0, 0x2B, 0x13, 0x3A, 0x65, 0xFF, 0x24, 0x72, 0xCF,
        0xA7, 0x6D, 0x52, 0x55, 0xF6, 0xC2, 0x30, 0x23, 0x7D, 0x9B, 0x9E, 0xB0, 0x94, 0x02, 0xAD, 0x60,
        0x8A, 0x9F, 0xBC, 0xC8, 0xE4, 0x2B, 0x92, 0x96, 0xF5, 0xAE, 0x04, 0xA4, 0x33, 0x0C, 0x90, 0x67,
        0xF0, 0xB9, 0x1E, 0x7E, 0xBE, 0x02, 0x18, 0xB2, 0x03, 0xB6, 0x40, 0xBF, 0x05, 0xE3, 0x76, 0x98,
        0x21, 0x38, 0xC9, 0x5F, 0xD3, 0x51, 0x8B, 0x43, 0x0B, 0x1A, 0x0B, 0xF9, 0x3C, 0x21, 0x6C, 0x3D,
        0xB8, 0xA0, 0x57, 0xCA, 0x68, 0xCD, 0x1E, 0xD2, 0x2C, 0x50, 0xEE, 0xC0, 0xDF, 0x25, 0x88, 0x52,
        0x37, 0xE1, 0x44, 0xC6, 0x76, 0x3B, 0x91, 0x95, 0x86, 0x76, 0x87, 0x49, 0x21, 0x93, 0x44, 0x0A,
        0x52, 0xB9, 0x2D, 0x2B, 0xE3, 0x1D, 0xB0, 0xE4, 0x98, 0xC6, 0xEE, 0x3D, 0x96, 0x53, 0x4B, 0xFF,
        0x39, 0x00, 0xD5, 0x42, 0x7E, 0xE1, 0x4C, 0x6F, 0xD5, 0xB7, 0xE6, 0x99, 0x2A, 0x5B, 0x67, 0xEE,
        0x3E, 0xBA, 0xF7, 0xEC, 0x43, 0x2A, 0x1C, 0xB6, 0xB5, 0x04, 0x26, 0x59, 0xB1, 0x4C, 0x17, 0xCC,
        0x83, 0xB9, 0x00, 0x3E, 0x36, 0x91, 0x90, 0xF7, 0x5E, 0x38, 0xDC, 0xE4, 0x15, 0xC7, 0x67, 0xF0,
        0xCA, 0xC8, 0xD2, 0x91, 0x5D, 0x74, 0xAC, 0x97, 0x56, 0x36, 0x1A, 0x82, 0x0A, 0xAA, 0xB4, 0x4E,
        0xBF, 0x29, 0x5C, 0xBF, 0x58, 0xB3, 0x97, 0xF9, 0xEB, 0x7C, 0x85, 0xB4, 0xA5, 0x13, 0x2F, 0xD1,
        0xDE, 0x1C, 0xEC, 0x97, 0xDD, 0xE2, 0x39, 0xE4, 0xFB, 0x0A, 0x02, 0xE0, 0xC3, 0xBA, 0x39, 0x79,
        0xAA, 0x1C, 0x37, 0x75, 0x25, 0x54, 0xBE, 0x85, 0x74, 0x2C, 0xFA, 0x0C, 0xFA, 0x50, 0xF6, 0xBE,
        0x9F, 0x2A, 0x53, 0x7C, 0x27, 0x46, 0x68, 0x2D, 0x74, 0x2B, 0x46, 0xDA, 0xF5, 0x07, 0x95, 0x09,
        0x6A, 0x91, 0xB7, 0xB1, 0x34, 0x07, 0x5F, 0xEA, 0xBE, 0x0F, 0x87, 0x28, 0x68, 0x97, 0x43, 0x77,
        0xD5, 0x38, 0x2B, 0x11, 0x11, 0x4F, 0xD9, 0x75, 0x5E, 0xE1, 0x06, 0xA0, 0x3B, 0xAC, 0x32, 0xFE,
        0xBF, 0x73, 0x59, 0x5B, 0xA2, 0xA8, 0x7E, 0x10, 0x4C, 0x6E, 0x78, 0xF0, 0x4A, 0x4E, 0x95, 0xD6,
        0xDD, 0x05, 0x7A, 0xBB, 0xF1, 0xEB, 0xA8, 0xA4, 0x5D, 0x91, 0xF0, 0xED, 0xDB, 0xB8, 0x01, 0x41,
        0xF8, 0x97, 0x7F, 0xC3, 0x91, 0x53, 0xBF, 0xE9, 0xEA, 0x33, 0x1F, 0xDC, 0xA6, 0xE6, 0x8D, 0xCB,
        0x75, 0xD0, 0x69, 0xD0, 0xA4, 0x59, 0xA5, 0x02, 0xFC, 0x60, 0x0D, 0x6A, 0xA0, 0x05, 0x1A, 0x54,
        0x8A, 0xA7, 0x57, 0xA3, 0xF0, 0x90, 0x8A, 0xD5, 0x6F, 0x1E, 0x2E, 0x10, 0x9A, 0x93, 0x2B, 0x51,
        0x2C, 0xFD, 0x99, 0xE5, 0x9B, 0x5D, 0xB2, 0xA7, 0x37, 0x99, 0x26, 0x35, 0xCA, 0xDD, 0x22, 0x19,
        0x59, 0x2A, 0xB0, 0x99, 0x23, 0xDF, 0xA7, 0xA9, 0x85, 0x12, 0xCF, 0xBF, 0xFC, 0x74, 0x80, 0x87,
        0xE1, 0x97, 0xD0, 0xF9, 0xEF, 0x5F, 0x1B, 0x45, 0xF7, 0x76, 0xDB, 0x66, 0x39, 0x05, 0x43, 0x06,
        0xA9, 0x9F, 0x2E, 0x14, 0x9F, 0x1C, 0x0C, 0x1F, 0xD5, 0xD9, 0xA4, 0x8D, 0x18, 0x6F, 0x08, 0x53,
        0x0B, 0x92, 0x9A, 0x0C, 0xEA, 0x4C, 0xE4, 0x1D, 0x9E, 0x9A, 0x51, 0xB8, 0x7E, 0x2D, 0xE7, 0x3C,
        0xFF, 0x84, 0x5C, 0xBF, 0x8F, 0x8C, 0x89, 0x09, 0x1B, 0x7E, 0x4B, 0xE7, 0x85, 0xEC, 0x04, 0xB5,
        0x20, 0x18, 0x1E, 0x55, 0xD5, 0x5B, 0xAC, 0xC6, 0x25, 0x5A, 0xA1, 0x81, 0xC1, 0x31, 0x9C, 0xF5,
        0xB5, 0x54, 0x07, 0x65, 0x0A, 0x5B, 0x90, 0x06, 0x4F, 0x84, 0xB2, 0x7F, 0xD1, 0xAD, 0x16, 0x81,
        0x25, 0xAF, 0xAF, 0xE2, 0x03, 0xA9, 0x1F, 0x13, 0x02, 0x5D, 0x54, 0x89, 0xCD, 0x44, 0x51, 0xEB,
        0xA4, 0x2B, 0xBD, 0x47, 0xB0, 0xB6, 0x27, 0x1D, 0x9B, 0x14, 0x6F, 0xBF, 0xCD, 0x59, 0xBC, 0x0A,
        0x37, 0xA8, 0x74, 0x7D, 0x16, 0x90, 0x28, 0xD5, 0x94, 0xC3, 0xE4, 0x23, 0xC4, 0x98, 0x91, 0xCE,
        0x55, 0xBD, 0x21, 0x3B, 0x84, 0xBD, 0x44, 0x3C, 0xF9, 0xCD, 0x37, 0x43, 0x4A, 0xC6, 0x8C, 0x23,
        0x04, 0x28, 0x63, 0x7A, 0x03, 0x85, 0xD2, 0x46, 0x93, 0xCA, 0xFE, 0xC3, 0x83, 0x0B, 0x13, 0xCC,
        0x5D, 0xCB, 0xBA, 0xCA, 0x68, 0xAB, 0x05, 0xF7, 0xEC, 0x4A, 0x9C, 0x0F, 0xD5, 0xC4, 0x5A, 0xA5,
        0xA0, 0x04, 0x41, 0x6A, 0xF6, 0xEF, 0x16, 0x9B, 0x69, 0x38, 0xF6, 0x2D, 0xAA, 0xEB, 0x2D, 0xE2,
        0x82, 0xA2, 0x9F, 0x6F, 0xBD, 0x2A, 0xE3, 0x66, 0x6B, 0x21, 0xDA, 0x56, 0xAD, 0x82, 0x2B, 0x93,
        0xF3, 0x25, 0xEA, 0xFC, 0xFD, 0xFD, 0x1B, 0xA9, 0xFC, 0xB8, 0xC6, 0x98, 0x45, 0xF2, 0x70, 0x03,
        0x4A, 0x9C, 0x60, 0x82, 0x65, 0xB6, 0x68, 0x4C, 0xE7, 0x41, 0x10, 0x9D, 0x59, 0x40, 0x03, 0x02,
        0x07, 0x12, 0x33, 0xAF, 0x79, 0xE1, 0xC4, 0xEB, 0xB8, 0xCE, 0x6A, 0x90, 0x72, 0x61, 0x5D, 0x56,
        0xC7, 0x59, 0x31, 0xCB, 0x45, 0x2D, 0x42, 0x9F, 0x10, 0x1D, 0x09, 0x63, 0x59, 0x8C, 0x6C, 0xDB,
        0x11, 0xCF, 0xA1, 0xDF, 0x5F, 0x4D, 0xDF, 0xB4, 0xC3, 0x82, 0xEE, 0x58, 0x16, 0xB4, 0x74, 0xFA,
        0xBE, 0x11, 0x9C, 0x1E, 0x98, 0x29, 0xDE, 0xE3, 0xE5, 0x9E, 0xCF, 0xD7, 0x91, 0x0A, 0xA3, 0xA4,
        0x42, 0xA1, 0x95, 0x09, 0x9E, 0x16, 0xD5, 0xA8, 0x24, 0x56, 0x5B, 0x23, 0xC8, 0x56, 0x4C, 0xCB,
        0x89, 0x18, 0x69, 0xEB, 0x0C, 0x1F, 0xC0, 0x41, 0x5C, 0x63, 0x04, 0x68, 0xB2, 0x0F, 0x3F, 0x88,
        0x36, 0xDD, 0x23, 0x4D, 0x4C, 0xC0, 0x81, 0xE3, 0xE9, 0xAD, 0xE0, 0x27, 0xD5, 0xE5, 0x46, 0xEB,
        0xFF, 0x32, 0xA2, 0xB7, 0x14, 0x64, 0x0B, 0x6D, 0x1B, 0xE5, 0xD8, 0xAE, 0x9D, 0xE8, 0x55, 0xB9,
        0x52, 0x70, 0x59, 0xB8, 0x72, 0x92, 0x69, 0x37, 0x95, 0x61, 0x0A, 0xE5, 0xF6, 0x55, 0x97, 0x1D,
        0xBF, 0xF7, 0x29, 0x77, 0x0F, 0x72, 0x80, 0xB2, 0x7E, 0x56, 0xBF, 0xFD, 0xE9, 0xF5, 0x9B, 0x62,
        0xE9, 0xBD, 0x0B, 0xC2, 0x07, 0x55, 0x31, 0x4C, 0x57, 0x3A, 0x05, 0xB9, 0x27, 0x41, 0x4A, 0xC3,
        0xEC, 0x72, 0x20, 0xB3, 0x0C, 0xF9, 0xD9, 0x3A, 0x14, 0x6A, 0x03, 0x44, 0x6A, 0xF1, 0x41, 0x55,
        0x7F, 0x81, 0xC2, 0x04, 0xA8, 0x05, 0xB9, 0x49, 0x2E, 0x43, 0xC4, 0x00, 0x87, 0x86, 0x04, 0xAC,
        0xAF, 0x73, 0x78, 0x0E, 0xA4, 0x43, 0x5B, 0x36, 0xA2, 0x8F, 0x9C, 0xF7, 0x66, 0x4A, 0x5A, 0x09,
        0x6B, 0xAA, 0x69, 0x6F, 0xB1, 0x20, 0x0D, 0x56, 0x85, 0x0A, 0x5E, 0x06, 0xBF, 0xE2, 0x32, 0xB4,
        0x5C, 0x46, 0x33, 0x0D, 0x27, 0xA3, 0x6B, 0xE1, 0xB2, 0x6A, 0x7D, 0x4A, 0xA7, 0x81, 0x0F, 0x2B,
        0x16, 0x7C, 0x51, 0xD6, 0xC0, 0x3D, 0xB9, 0xFE, 0xB4, 0x66, 0xC4, 0xB6, 0x54, 0x53, 0x67, 0xDA,
        0x70, 0x96, 0x9A, 0x0A, 0x07, 0x1A, 0x26, 0xBA, 0x85, 0x50, 0xF5, 0x27, 0x53, 0x9C, 0x3A, 0x94,
        0x0A, 0x7D, 0xDB, 0xE1, 0xC3, 0xE3, 0x6A, 0x3E, 0x9E, 0xD5, 0x13, 0x0A, 0xA3, 0xD2, 0x21, 0x75,
        0x79, 0x17, 0x26, 0xAC, 0x48, 0x5F, 0x3D, 0xE1, 0x7D, 0xA4, 0xB1, 0x56, 0x0F, 0x92, 0x2C, 0x60,
        0xE6, 0xCB, 0x87, 0x35, 0xB8, 0x75, 0xC3, 0xA2, 0x03, 0x50, 0x4B, 0xA2, 0x6E, 0x01, 0xE1, 0xDD,
        0x87, 0xA5, 0x33, 0xC6, 0x2F, 0xA2, 0x41, 0xFC, 0x72, 0x98, 0xA2, 0x69, 0x4C, 0x3F, 0xF0, 0x53,
        0xF5, 0x41, 0x2B, 0x23, 0x24, 0x3B, 0xCE, 0x9D, 0x39, 0x31, 0x17, 0x08, 0xE1, 0x3F, 0x5F, 0xFB,
        0x00, 0xFA, 0xF1, 0xE3, 0xE1, 0x7B, 0x0C, 0xDF, 0x8D, 0xA2, 0xC4, 0xCD, 0x62, 0x3D, 0xAE, 0xC7,
        0x48, 0x09, 0x1C, 0x66, 0xCB, 0x0E, 0x23, 0xE8, 0x1B, 0x9F, 0x1B, 0xCB, 0xF8, 0x14, 0xC3, 0x34,
        0x91, 0x32, 0x2B, 0x39, 0x1C, 0xBA, 0x1C, 0xA0, 0x19, 0xF2, 0x57, 0x9D, 0x78, 0x00, 0x55, 0x1F,
        0x15, 0x12, 0x9A, 0xA2, 0xF2, 0xC2, 0xB7, 0x4E, 0xEA, 0x46, 0x01, 0xC2, 0xE9, 0x76, 0xBF, 0xDE,
        0xCF, 0x8B, 0xC7, 0x50, 0x80, 0xEE, 0x46, 0x91, 0x93, 0x1E, 0x5C, 0x48, 0x5D, 0xC8, 0xC8, 0x63,
        0xD1, 0x89, 0x02, 0x29, 0xE9, 0x90, 0x9F, 0x0B, 0x0A, 0x1A, 0x44, 0x17, 0xE7, 0x4E, 0xAD, 0x58,
        0x55, 0xF8, 0x38, 0xF6, 0x4F, 0xD8, 0x1C, 0x7E, 0x25, 0x9B, 0x59, 0x16, 0xBC, 0x65, 0x24, 0xC5,
        0xA7, 0x56, 0xE5, 0x20, 0x3F, 0xD9, 0x27, 0xE0, 0x32, 0x24, 0xE1, 0x7B, 0xE1, 0x32, 0xEA, 0xF4,
        0xFE, 0xD9, 0xA5, 0xFF, 0x35, 0xAE, 0xA9, 0x1B, 0x38, 0x28, 0x6A, 0xC0, 0x1A, 0x42, 0xD9, 0x5E,
        0x14, 0x2C, 0xC2, 0x2D, 0x9B, 0x94, 0x5B, 0xCF, 0x83, 0x30, 0xB9, 0x06, 0xAF, 0x4B, 0xD7, 0xF6,
        0x38, 0x7C, 0xFF, 0xB4, 0xA5, 0x1A, 0xA0, 0xE9, 0xF3, 0x01, 0xE3, 0x97, 0xC4, 0xA9, 0x57, 0xF5,
        0xB9, 0x96, 0xA7, 0xA3, 0xB8, 0x10, 0x0E, 0xFB, 0x1D, 0x39, 0x44, 0x16, 0x97, 0x94, 0x3E, 0x5F,
        0xAF, 0x0F, 0xE3, 0x99, 0xDC, 0xA0, 0xE9, 0x8D, 0x26, 0x2B, 0xD9, 0xAE, 0xEC, 0x4C, 0x4F, 0x09,
        0x86, 0x7E, 0x7B, 0xC3, 0xE3, 0xC6, 0x17, 0xAE, 0x30, 0x9C, 0x31, 0xD1, 0x84, 0x47, 0xAF, 0xCB,
        0xEA, 0x69, 0x2A, 0x08, 0x3E, 0x13, 0x00, 0xDE, 0xF6, 0x4A, 0x42, 0xD3, 0xBE, 0x33, 0xD9, 0x50,
        0x6B, 0x8D, 0x59, 0x12, 0x1A, 0xD3, 0xA7, 0x7C, 0x0A, 0xE7, 0x87, 0x47, 0xCA, 0xAA, 0x33, 0xFD,
        0xC1, 0xF6, 0x28, 0xC1, 0x62, 0xA2, 0x4C, 0x79, 0x83, 0x48, 0x86, 0x0E, 0xA4, 0x67, 0x34, 0x95,
        0xAE, 0x7D, 0xD6, 0xEE, 0x91, 0x05, 0x35, 0x91, 0xE8, 0x34, 0x39, 0xA3, 0xE5, 0xE6, 0x80, 0x53,
        0x76, 0x1F, 0x94, 0xA0, 0xF6, 0xA5, 0x41, 0x79, 0x82, 0xD3, 0xB0, 0x1F, 0xCE, 0xE1, 0x86, 0x64,
        0x65, 0x0C, 0x8D, 0xD6, 0xFA, 0xC1, 0x10, 0x6C, 0x07, 0xD5, 0xF0, 0x77, 0x65, 0xB9, 0x0C, 0xBD,
        0xAE, 0x2D, 0x62, 0x6C, 0x42, 0x7E, 0x2A, 0xBE, 0x5F, 0xC1, 0x17, 0x3B, 0x07, 0xFF, 0x5E, 0xD7,
        0x31, 0x52, 0x26, 0x2F, 0x9F, 0x12, 0xD8, 0x2E, 0xA3, 0xF5, 0xB5, 0xD2, 0xFC, 0x6E, 0x08, 0x1F,
        0xC8, 0x93, 0xA1, 0xEB, 0xF9, 0x13, 0x1D, 0x1F, 0x98, 0x5E, 0xB0, 0x0C, 0x65, 0x6C, 0xAE, 0x07,
        0x78, 0xF8, 0x12, 0xD2, 0xD1, 0x1E, 0x77, 0x5C, 0x24, 0x62, 0xE5, 0x94, 0xD6, 0x6A, 0x8E, 0xD0,
        0x72, 0x59, 0xDA, 0x48, 0x38, 0x2F, 0x31, 0x75, 0x0C, 0x52, 0xF0, 0x0C, 0x8F, 0x5C, 0xE9, 0x5E,
        0x5A, 0x94, 0xE8, 0xD2, 0x80, 0xF8, 0x4F, 0xE7, 0xAA, 0x6C, 0xBE, 0x47, 0xFB, 0xDD, 0x57, 0x0A,
        0xD8, 0x5E, 0xCC, 0x0D, 0x8F, 0x42, 0x5E, 0xDC, 0x5D, 0x95, 0x95, 0x60, 0x9B, 0x6F, 0x05, 0x5E,
        0x08, 0x45, 0x91, 0xE4, 0xB8, 0x06, 0xB1, 0xF2, 0xC0, 0xD7, 0xE3, 0x47, 0xB7, 0x38, 0x08, 0xA8,
        0x58, 0xE4, 0x55, 0xFC, 0xE2, 0x37, 0x1F, 0x38, 0xA2, 0x18, 0x9E, 0xC2, 0x0F, 0x90, 0x14, 0x20,
        0x50, 0xD1, 0xD0, 0xAB, 0x36, 0x7F, 0xAA, 0x03, 0x1C, 0xE6, 0x0A, 0xF9, 0x8E, 0x41, 0xDB, 0x32,
        0x1C, 0x68, 0xA0, 0xA0, 0xED, 0x4A, 0xF4, 0x4B, 0x09, 0xD0, 0xF0, 0x01, 0x8B, 0x17, 0x44, 0xE1,
        0xEA, 0xC5, 0x9D, 0x3B, 0x37, 0x7A, 0x68, 0xF1, 0x78, 0x46, 0xCF, 0xB6, 0x57, 0xDB, 0x4B, 0x5C,
        0x03, 0xE1, 0x9D, 0xC0, 0x37, 0x55, 0x8D, 0x03, 0xFB, 0x6A, 0x00, 0x82, 0x19, 0xD1, 0xC0, 0x76,
        0x97, 0xEE, 0xC9, 0xAD, 0x0D, 0x72, 0x0B, 0xE9, 0xA8, 0x09, 0x92, 0x03, 0xA4, 0xAA, 0x2C, 0xCF,
        0xFD, 0xDE, 0x86, 0xD0, 0x06, 0x4A, 0xAE, 0x7E, 0xC1, 0xB8, 0x2A, 0x4E, 0x9F, 0xA3, 0x5E, 0x8C,
        0x12, 0x40, 0x74, 0x38, 0xE7, 0xEA, 0xB0, 0x51, 0xC2, 0xB9, 0x6D, 0x4A, 0x50, 0xBF, 0x59, 0x9C,
        0x05, 0xB2, 0x42, 0xE2, 0x0F, 0x71, 0x44, 0xDB, 0x97, 0x0B, 0xD0, 0xDB, 0x44, 0x1F, 0x9A, 0x3B,
        0x18, 0x2A, 0x7B, 0xD9, 0x03, 0x83, 0x0B, 0xCF, 0x27, 0x20, 0x43, 0xA6, 0x42, 0xED, 0x89, 0x63,
        0xDB, 0x2D, 0x27, 0xC2, 0x3B, 0xE6, 0x0D, 0x3E, 0xB6, 0x96, 0x33, 0x70, 0xA6, 0xF3, 0xF5, 0x56,
        0xEA, 0xEB, 0xF1, 0xE7, 0xD8, 0xCB, 0x04, 0x48, 0x99, 0x4C, 0x00, 0xA4, 0x2A, 0xA5, 0x8A, 0xF1,
        0x58, 0xD5, 0x17, 0x4C, 0xC5, 0x88, 0x06, 0x8F, 0xA6, 0x67, 0xA6, 0x14, 0xC7, 0xB9, 0xE0, 0x86,
        0xAC, 0x67, 0xFD, 0xB3, 0x5B, 0x3E, 0xDF, 0x03, 0xFD, 0xC8, 0xC4, 0x4A, 0x32, 0x78, 0x6B, 0xD1,
        0xC1, 0xE2, 0x36, 0x9D, 0x0B, 0xF2, 0x54, 0x25, 0xB8, 0xB7, 0xB2, 0x10, 0x7A, 0xA6, 0x79, 0x52,
        0xC2, 0xEE, 0x98, 0xA5, 0x3D, 0xF0, 0x07, 0x8D, 0x25, 0xC3, 0xAC, 0xFD, 0xCF, 0x83, 0x98, 0x80,
        0x56, 0x95, 0xC4, 0x14, 0xA2, 0xA5, 0x93, 0xFE, 0x24, 0x59, 0x44, 0x73, 0xDF, 0xD6, 0x47, 0xDA,
        0x22, 0x3A, 0x82, 0xC5, 0xD1, 0x59, 0x40, 0x9D, 0x0C, 0x1A, 0xB7, 0x79, 0x45, 0x9A, 0xF8, 0x6D,
        0x5A, 0x5C, 0xC2, 0x80, 0xFC, 0xAA, 0x8A, 0xA4, 0xFE, 0x68, 0x61, 0x7D, 0xFE, 0x2C, 0x36, 0xE3,
        0xE0, 0x59, 0x28, 0x40, 0x79, 0xAD, 0x2D, 0x28, 0x12, 0x30, 0xFC, 0x56, 0x2E, 0x1D, 0xEC, 0x48,
        0x3A, 0xF0, 0xC5, 0x6C, 0x31, 0xE0, 0x2E, 0xB3, 0x91, 0x70, 0xB9, 0x9E, 0xBD, 0xE7, 0x96, 0x58,
        0xCB, 0xBC, 0x1C, 0xE4, 0xC7, 0x78, 0xC7, 0x1E, 0x39, 0xDB, 0xB8, 0x77, 0x50, 0xB7, 0x65, 0x20,
        0x04, 0x16, 0x8B, 0xFC, 0x66, 0xC4, 0x6D, 0x05, 0x8C, 0x3C, 0xB6, 0x32, 0x2F, 0xDE, 0xC3, 0x6F,
        0xFC, 0x82, 0x06, 0x02, 0x87, 0x47, 0xFD, 0xD8, 0xDA, 0x75, 0xE0, 0x4E, 0x8C, 0x40, 0x00, 0xB2,
        0x9B, 0x35, 0x78, 0xA4, 0x61, 0x64, 0x96, 0x62, 0x37, 0xF6, 0x3E, 0x39, 0xFA, 0x14, 0x5B, 0xC4,
        0x70, 0x17, 0xDC, 0x0C, 0x9E, 0x31, 0x82, 0x2C, 0x63, 0xCC, 0x8A, 0x43, 0x7C, 0x69, 0x12, 0x05,
        0x18, 0xA3, 0x62, 0xCC, 0xA2, 0x13, 0x96, 0x25, 0xA6, 0x1B, 0xF2, 0x10, 0xC8, 0x73, 0x4F, 0xCB,
        0x80, 0xCA, 0xAF, 0x73, 0xC9, 0x78, 0xB1, 0xAE, 0x87, 0xB8, 0xDF, 0x50, 0xD3, 0x55, 0x1E, 0x3A,
        0x81, 0xF6, 0x84, 0xD6, 0x57, 0x36, 0xCF, 0x38, 0xB7, 0xBC, 0xBC, 0x1E, 0x48, 0x62, 0x9F, 0x0F,
        0x0C, 0xE5, 0xF0, 0x63, 0x33, 0xE6, 0x59, 0x6B, 0x1E, 0xE6, 0x1C, 0x8A, 0xF9, 0xDD, 0x6B, 0xA3,
        0xDC, 0x02, 0x4A, 0x2F, 0x8C, 0x6A, 0x8D, 0x16, 0x7E, 0x2F, 0xF1, 0x75, 0xD5, 0x15, 0x93, 0x07,
        0x27, 0xD9, 0x6F, 0x1A, 0x5D, 0x43, 0xF3, 0x47, 0xC4, 0xED, 0xAD, 0x05, 0x9F, 0xEC, 0x8F, 0xD0,
        0xBE, 0xB5, 0x58, 0xF4, 0xF6, 0xBE, 0x08, 0x73, 0x96, 0x19, 0x05, 0x25, 0xEC, 0x3D, 0x26, 0xF4,
        0x93, 0xDB, 0x9F, 0x56, 0x48, 0x4C, 0xBC, 0xD0, 0x02, 0x59, 0xD1, 0x40, 0x4C, 0xA6, 0x06, 0x41,
        0xE8, 0x7D, 0x47, 0xAE, 0x3A, 0x9E, 0x1A, 0x71, 0x52, 0xD4, 0x67, 0xC1, 0x14, 0x7E, 0x40, 0x6F,
        0x1C, 0x75, 0x30, 0x7B, 0x70, 0x3A, 0xE0, 0x37, 0xB7, 0x41, 0x7F, 0xCB, 0x4A, 0xBA, 0xA7, 0xCE,
        0x56, 0x54, 0xC5, 0x46, 0x65, 0x6F, 0xB4, 0xB6, 0xF0, 0x57, 0xCE, 0x2E, 0x4F, 0xA9, 0xF0, 0x14,
        0x50, 0xC3, 0x30, 0xC5, 0xBA, 0xE1, 0x5E, 0xD6, 0xDC, 0xC5, 0x78, 0x55, 0x32, 0xAA, 0xCB, 0x29,
        0x35, 0x81, 0x46, 0x5E, 0x92, 0xE7, 0xDE, 0xCC, 0x92, 0x29, 0x86, 0xE0, 0x8F, 0x91, 0x3C, 0x74,
        0x97, 0x79, 0x63, 0x97, 0x4A, 0xCC, 0x88, 0xB5, 0xA3, 0x7A, 0xF0, 0xF0, 0x33, 0x87, 0xCD, 0xBD
    };
    uint8_t b = Value ^ hdSurEncBasicCrypt(Value, lookup[(((AddrInd * 2) + 0 ) * 256) + Value]);
    return (Value ^ hdSurEncBasicCrypt(b, lookup[(((AddrInd * 2) + 1) * 256) + b]));
}

void hdSurEncPhase1(uint8_t *CWs)
{
    static const uint8_t lookup1[] =
    {
        0x16, 0x71, 0xCA, 0x14, 0xC4, 0xF4, 0xA3, 0x5A, 0x9D, 0x5F, 0x85, 0x8B, 0xA6, 0x77, 0xFD, 0x3C,
        0x5F, 0x13, 0x2A, 0x5F, 0x61, 0x36, 0xE4, 0xDC, 0x0D, 0x82, 0x92, 0xC5, 0x25, 0xE1, 0x7A, 0x1C,
        0x29, 0x19, 0x94, 0x2F, 0xC5, 0xD2, 0xDC, 0xBA, 0x86, 0x60, 0x64, 0x60, 0x86, 0x92, 0xA3, 0x4E,
        0x3D, 0x9B, 0xCC, 0x16, 0xBB, 0xBA, 0xD2, 0xF0, 0x6A, 0xD3, 0x2F, 0x07, 0x75, 0xBD, 0x28, 0xDB
    };
    static const int8_t lookup2[] = {1, -1, -1, 1, -1, 2, 1, -2, -1, 1, 2, -2, 1, -2, -2, 4};
    static const int8_t CAddrIndex[] = {0, 1, 3, 4};
    int32_t i, j, i1, i2, i3;

    for (i = 3; i >= 0; --i)
    {
        for (j = 0; j <= 15; ++j)
        {
            CWs[j] = CWs[j] ^ hdSurEncBasicCrypt(j , lookup1 [(16 * i) + j]);
        }

        uint8_t Buffer[16];
        uint32_t k;
        for (i1 = 0; i1 <= 3; ++i1)
        {
            for (i2 = 0; i2 <= 3; ++i2)
            {
                k = 0;
                for (i3 = 0; i3 <= 3; ++i3)
                {
                    k = k + (CWs[(i2 * 4) + i3] * lookup2[(i3 * 4) + i1]);
                    Buffer[(i2 * 4) + i1] = (uint8_t)k;
                }
            }
        }
        memcpy (CWs, Buffer, 16);

        // CW positions are mixed around here
        uint8_t a4[4];
        for (i1 = 1; i1 <= 3; ++i1)
        {
            for (i2 = 0; i2 <= 3; ++i2)
            {
                a4[i2] = i1 + (i2 * 4);
            }
            for (i2 = 0; i2 <= i1 - 1; ++i2)        // the given code in Func1_3 seems to be wrong here(3 instead of i1-1)!
            {
                uint8_t tmp = CWs[a4[0]];
                for (i3 = 1; i3 <= 3; ++i3)
                {
                    CWs[a4[i3 - 1]] = CWs[a4[i3]];
                }
                CWs[a4[3]] = tmp;
            }
        }

        for (i1 = 0; i1 <= 15; ++i1)
        {
            CWs[i1] = hdSurEncCryptLookup(CWs[i1], CAddrIndex[i1 & 3]);
        }
    }
}

void hdSurEncPhase2_sub(uint8_t *CWa, uint8_t *CWb, uint8_t AddrInd )
{
    uint8_t Buffer[8];
    uint8_t tmp, i;
    for (i = 0; i <= 7; ++i)
    {
        Buffer[i] = hdSurEncCryptLookup(CWb[i], AddrInd);
    }

    // some bitshifting
    tmp = Buffer[7];
    for (i = 7; i >= 1; --i)
    {
        Buffer[i] = ((Buffer[1] >> 4) & 0xFF) | ((Buffer[i - 1] << 4) & 0xFF);
    }
    Buffer[0] = ((Buffer[0] >> 4) & 0xFF ) | ((tmp << 4) & 0xFF);

    // saving the result
    for (i = 0; i <= 7; ++i)
    {
        CWa[i] = CWa[i] ^ Buffer[i];
    }
}

void hdSurEncPhase2(uint8_t *CWs)
{
    hdSurEncPhase2_sub(CWs, CWs + 8, 0);
    hdSurEncPhase2_sub(CWs + 8, CWs, 1);
    hdSurEncPhase2_sub(CWs, CWs + 8, 2);
    hdSurEncPhase2_sub(CWs + 8, CWs, 3);
    hdSurEncPhase2_sub(CWs, CWs + 8, 4);
}

static int32_t viaccess_card_init(struct s_reader *reader, ATR *newatr)
{
    get_atr;
    def_resp;
    int32_t i;
    uchar buf[256];
    uchar insac[] = { 0xca, 0xac, 0x00, 0x00, 0x00 }; // select data
    uchar insb8[] = { 0xca, 0xb8, 0x00, 0x00, 0x00 }; // read selected data
    uchar insa4[] = { 0xca, 0xa4, 0x00, 0x00, 0x00 }; // select issuer
    uchar insc0[] = { 0xca, 0xc0, 0x00, 0x00, 0x00 }; // read data item
    static const uchar insFAC[] = { 0x87, 0x02, 0x00, 0x00, 0x03 }; // init FAC
    static const uchar FacDat[] = { 0x00, 0x00, 0x28 };
    static unsigned char ins8702_data[] = { 0x00, 0x00, 0x11};
    static unsigned char ins8704[] = { 0x87, 0x04, 0x00, 0x00, 0x07 };
    static unsigned char ins8706[] = { 0x87, 0x06, 0x00, 0x00, 0x04 };


    if ((atr[1] != 0x77) || ((atr[2] != 0x18) && (atr[2] != 0x11) && (atr[2] != 0x19)) || ((atr[9] != 0x68) && (atr[9] != 0x6C)))
        return ERROR;

    write_cmd(insFAC, FacDat);
    if ( !(cta_res[cta_lr - 2] == 0x90 && cta_res[cta_lr - 1] == 0) )
        return ERROR;

    if (!cs_malloc(&reader->csystem_data, sizeof(struct viaccess_data)))
        return ERROR;
    struct viaccess_data *csystem_data = reader->csystem_data;

    write_cmd(insFAC, ins8702_data);
    if ((cta_res[cta_lr - 2] == 0x90) && (cta_res[cta_lr - 1] == 0x00))
    {
        write_cmd(ins8704, NULL);
        if ((cta_res[cta_lr - 2] == 0x90) && (cta_res[cta_lr - 1] == 0x00))
        {
            write_cmd(ins8706, NULL);
            if ((cta_res[cta_lr - 2] == 0x90) && (cta_res[cta_lr - 1] == 0x00))
            {
                csystem_data->last_geo.number_ecm = (cta_res[2] << 8) | (cta_res[3]);
                rdr_log(reader,  "using ecm #%x for long viaccess ecm", csystem_data->last_geo.number_ecm);
            }
        }
    }


    //  switch((atr[atrsize-4]<<8)|atr[atrsize-3])
    //  {
    //    case 0x6268: ver="2.3"; break;
    //    case 0x6668: ver="2.4(?)"; break;
    //    case 0xa268:
    //    default: ver="unknown"; break;
    //  }

    reader->caid = 0x500;
    memset(reader->prid, 0xff, sizeof(reader->prid));
    insac[2] = 0xa4; write_cmd(insac, NULL); // request unique id
    insb8[4] = 0x07; write_cmd(insb8, NULL); // read unique id
    memcpy(reader->hexserial, cta_res + 2, 5);
    //  rdr_log(reader, "[viaccess-reader] type: Viaccess, ver: %s serial: %llu", ver, b2ll(5, cta_res+2));
    rdr_log_sensitive(reader, "type: Viaccess (%sstandard atr), caid: %04X, serial: {%llu}",
                      atr[9] == 0x68 ? "" : "non-", reader->caid, (unsigned long long) b2ll(5, cta_res + 2));

    i = 0;
    insa4[2] = 0x00; write_cmd(insa4, NULL); // select issuer 0
    buf[0] = 0;
    while ((cta_res[cta_lr - 2] == 0x90) && (cta_res[cta_lr - 1] == 0))
    {
        insc0[4] = 0x1a; write_cmd(insc0, NULL); // show provider properties
        cta_res[2] &= 0xF0;
        reader->prid[i][0] = 0;
        memcpy(&reader->prid[i][1], cta_res, 3);
        memcpy(&csystem_data->availkeys[i][0], cta_res + 10, 16);
        snprintf((char *)buf + strlen((char *)buf), sizeof(buf) - strlen((char *)buf), ",%06X", b2i(3, &reader->prid[i][1]));
        //rdr_log(reader, "[viaccess-reader] buf: %s", buf);

        insac[2] = 0xa5; write_cmd(insac, NULL); // request sa
        insb8[4] = 0x06; write_cmd(insb8, NULL); // read sa
        memcpy(&reader->sa[i][0], cta_res + 2, 4);

        /*
        insac[2]=0xa7; write_cmd(insac, NULL); // request name
        insb8[4]=0x02; write_cmd(insb8, NULL); // read name nano + len
        l=cta_res[1];
        insb8[4]=l; write_cmd(insb8, NULL); // read name
        cta_res[l]=0;
        rdr_log(reader, "[viaccess-reader] name: %s", cta_res);
        */

        insa4[2] = 0x02;
        write_cmd(insa4, NULL); // select next issuer
        i++;
    }
    reader->nprov = i;
    rdr_log(reader, "providers: %d (%s)", reader->nprov, buf + 1);

    if (cfg.ulparent)
        unlock_parental(reader);

    rdr_log(reader, "ready for requests");
    return OK;
}

bool dcw_crc(uchar *dw)
{
    int8_t i;
    for (i = 0; i < 16; i += 4) if (dw[i + 3] != ((dw[i] + dw[i + 1] + dw[i + 2]) & 0xFF))return 0;
    return 1;
}

static int32_t viaccess_do_ecm(struct s_reader *reader, const ECM_REQUEST *er, struct s_ecm_answer *ea)
{
    def_resp;
    static const unsigned char insa4[] = { 0xca, 0xa4, 0x04, 0x00, 0x03 }; // set provider id
    unsigned char ins88[] = { 0xca, 0x88, 0x00, 0x00, 0x00 }; // set ecm
    unsigned char insf8[] = { 0xca, 0xf8, 0x00, 0x00, 0x00 }; // set geographic info
    static const unsigned char insc0[] = { 0xca, 0xc0, 0x00, 0x00, 0x12 }; // read dcw
    struct viaccess_data *csystem_data = reader->csystem_data;

    // //XXX what is the 4th byte for ??
    int32_t ecm88Len = MIN(MAX_ECM_SIZE - 4, SCT_LEN(er->ecm) - 4);
    if (ecm88Len < 1)
    {
        rdr_log(reader, "ECM: Size of ECM couldn't be correctly calculated.");
        return ERROR;
    }
    uchar ecmData[ecm88Len];
    memset(ecmData, 0, ecm88Len);
    memcpy(ecmData, er->ecm + 4, ecm88Len);
    uchar *ecm88Data = &ecmData[0];
    uint32_t provid = 0;
    int32_t rc = 0;
    int32_t hasD2 = 0;
    int32_t curEcm88len = 0;
    int32_t nanoLen = 0;
    uchar *nextEcm;
    uchar DE04[256];
    int32_t D2KeyID = 0;
    int32_t curnumber_ecm = 0;
    //nanoD2 d2 02 0d 02 -> D2 nano, len 2
    // 0d, 0f -> post AES decrypt CW
    // 0b, 11 -> pre AES decrypt CW
    int32_t nanoD2 = 0; //   0x0b = 1  0x0d = 2 0x0f = 3 0x11 = 4

    memset(DE04, 0, sizeof(DE04)); //fix dorcel de04 bug

    nextEcm = ecm88Data;



    while (ecm88Len > 0 && !rc)
    {

        if (ecm88Data[0] == 0x00 &&  ecm88Data[1] == 0x00)
        {
            // nano 0x00  and len 0x00 aren't valid ... something is obviously wrong with this ecm.
            rdr_log(reader, "ECM: Invalid ECM structure. Rejecting");
            return ERROR;
        }

        // 80 33 nano 80 (ecm) + len (33)
        if (ecm88Data[0] == 0x80) // nano 80, give ecm len
        {
            curEcm88len = ecm88Data[1];
            nextEcm = ecm88Data + curEcm88len + 2;
            ecm88Data += 2;
            ecm88Len -= 2;
        }

        if (!curEcm88len)  //there was no nano 80 -> simple ecm
        {
            curEcm88len = ecm88Len;
        }

        // d2 02 0d 02 -> D2 nano, len 2,  select the AES key to be used
        if (ecm88Data[0] == 0xd2)
        {
            // test if need post or pre AES decrypt
            if (ecm88Data[2] == 0x0b)
            {
                nanoD2 = 1;
                rdr_debug_mask(reader, D_READER, "ECM: nano D2 0x0b");
            }
            if (ecm88Data[2] == 0x0d)
            {
                nanoD2 = 2;
                rdr_debug_mask(reader, D_READER, "ECM: nano D2 0x0d");
            }
            if (ecm88Data[2] == 0x0f)
            {
                nanoD2 = 3;
                rdr_debug_mask(reader, D_READER, "ECM: nano D2 0x0f");
            }
            if (ecm88Data[2] == 0x11)
            {
                nanoD2 = 4;
                rdr_debug_mask(reader, D_READER, "ECM: nano D2 0x11");
            }
            // use the d2 arguments to get the key # to be used
            int32_t len = ecm88Data[1] + 2;
            D2KeyID = ecm88Data[3];
            ecm88Data += len;
            ecm88Len -= len;
            curEcm88len -= len;
            hasD2 = 1;
        }
        else
            hasD2 = 0;


        // 40 07 03 0b 00  -> nano 40, len =7  ident 030B00 (tntsat), key #0  <== we're pointing here
        // 09 -> use key #9
        // 05 67 00
        if ((ecm88Data[0] == 0x90 || ecm88Data[0] == 0x40) && (ecm88Data[1] == 0x03 || ecm88Data[1] == 0x07 ) )
        {
            uchar ident[3], keynr;
            uchar *ecmf8Data = 0;
            int32_t ecmf8Len = 0;

            nanoLen = ecm88Data[1] + 2;
            keynr = ecm88Data[4] & 0x0F;

            // 40 07 03 0b 00  -> nano 40, len =7  ident 030B00 (tntsat), key #0  <== we're pointing here
            // 09 -> use key #9
            if (nanoLen > 5)
            {
                curnumber_ecm = (ecm88Data[6] << 8) | (ecm88Data[7]);
                rdr_debug_mask(reader, D_READER, "checking if the ecm number (%x) match the card one (%x)", curnumber_ecm, csystem_data->last_geo.number_ecm);
                // if we have an ecm number we check it.
                // we can't assume that if the nano len is 5 or more we have an ecm number
                // as some card don't support this
                if ( csystem_data->last_geo.number_ecm > 0 )
                {
                    if (csystem_data->last_geo.number_ecm == curnumber_ecm && !( ecm88Data[nanoLen - 1] == 0x01 && (ecm88Data[2] == 0x03 && ecm88Data[3] == 0x0B && ecm88Data[4] == 0x00 ) ))
                    {
                        keynr = ecm88Data[5];
                        rdr_debug_mask(reader, D_READER, "keyToUse = %02x, ECM ending with %02x", ecm88Data[5], ecm88Data[nanoLen - 1]);
                    }
                    else
                    {
                        if ( ecm88Data[nanoLen - 1] == 0x01 && (ecm88Data[2] == 0x03 && ecm88Data[3] == 0x0B && ecm88Data[4] == 0x00 ) )
                        {
                            rdr_debug_mask(reader, D_READER, "Skip ECM ending with = %02x for ecm number (%x) for provider %02x%02x%02x", ecm88Data[nanoLen - 1], curnumber_ecm, ecm88Data[2], ecm88Data[3], ecm88Data[4]);
                        }
                        rdr_debug_mask(reader, D_READER, "Skip ECM ending with = %02x for ecm number (%x)", ecm88Data[nanoLen - 1], curnumber_ecm);
                        ecm88Data = nextEcm;
                        ecm88Len -= curEcm88len;
                        continue; //loop to next ecm
                    }
                }
                else   // long ecm but we don't have an ecm number so we have to try them all.
                {
                    keynr = ecm88Data[5];
                    rdr_debug_mask(reader, D_READER, "keyToUse = %02x", ecm88Data[5]);
                }
            }

            memcpy (ident, &ecm88Data[2], sizeof(ident));
            provid = b2i(3, ident);
            ident[2] &= 0xF0;

            if (hasD2 && reader->aes_list)
            {
                // check that we have the AES key to decode the CW
                // if not there is no need to send the ecm to the card
                if (!aes_present(reader->aes_list, 0x500, (uint32_t) (provid & 0xFFFFF0) , D2KeyID))
                    return ERROR;
            }


            if (!chk_prov(reader, ident, keynr))
            {
                rdr_debug_mask(reader, D_READER, "ECM: provider or key not found on card");
                snprintf( ea->msglog, MSGLOGSIZE, "provider(%02x%02x%02x) or key(%d) not found on card", ident[0], ident[1], ident[2], keynr );
                return ERROR;
            }

            ecm88Data += nanoLen;
            ecm88Len -= nanoLen;
            curEcm88len -= nanoLen;

            // DE04
            if (ecm88Data[0] == 0xDE && ecm88Data[1] == 0x04)
            {
                memcpy (DE04, &ecm88Data[0], 6);
                ecm88Data += 6;
            }
            //

            if ( csystem_data->last_geo.provid != provid )
            {
                csystem_data->last_geo.provid = provid;
                csystem_data->last_geo.geo_len = 0;
                csystem_data->last_geo.geo[0]  = 0;
                write_cmd(insa4, ident); // set provider
            }

            //Nano D2 0x0b and 0x0f Pre AES decrypt CW
            if ( hasD2 && (nanoD2 == 1 || nanoD2 == 3))
            {
                uchar *ecm88DataCW = ecm88Data;
                int32_t cwStart = 0;
                //int32_t cwStartRes = 0;
                int32_t must_exit = 0;
                // find CW start
                while (cwStart < curEcm88len - 1 && !must_exit)
                {
                    if (ecm88Data[cwStart] == 0xEA && ecm88Data[cwStart + 1] == 0x10)
                    {
                        ecm88DataCW = ecm88DataCW + cwStart + 2;
                        must_exit = 1;
                    }
                    cwStart++;
                }
                if (nanoD2 == 3)
                {
                    hdSurEncPhase1(ecm88DataCW);
                    hdSurEncPhase2(ecm88DataCW);
                }
                // use AES from list to decrypt CW
                rdr_debug_mask(reader, D_READER, "Decoding CW : using AES key id %d for provider %06x", D2KeyID, (provid & 0xFFFFF0));
                if (aes_decrypt_from_list(reader->aes_list, 0x500, (uint32_t) (provid & 0xFFFFF0), D2KeyID, &ecm88DataCW[0], 16) == 0)
                    snprintf( ea->msglog, MSGLOGSIZE, "AES Decrypt : key id %d not found for CAID %04X , provider %06x", D2KeyID, 0x500, (provid & 0xFFFFF0) );
                if (nanoD2 == 3)
                {
                    hdSurEncPhase1(ecm88DataCW);
                }
            }

            while (ecm88Len > 1 && ecm88Data[0] < 0xA0)
            {
                nanoLen = ecm88Data[1] + 2;
                if (!ecmf8Data)
                    ecmf8Data = (uchar *)ecm88Data;
                ecmf8Len += nanoLen;
                ecm88Len -= nanoLen;
                curEcm88len -= nanoLen;
                ecm88Data += nanoLen;
            }
            if (ecmf8Len)
            {
                if ( csystem_data->last_geo.geo_len != ecmf8Len ||
                        memcmp(csystem_data->last_geo.geo, ecmf8Data, csystem_data->last_geo.geo_len))
                {
                    memcpy(csystem_data->last_geo.geo, ecmf8Data, ecmf8Len);
                    csystem_data->last_geo.geo_len = ecmf8Len;
                    insf8[3] = keynr;
                    insf8[4] = ecmf8Len;
                    write_cmd(insf8, ecmf8Data);
                }
            }
            ins88[2] = ecmf8Len ? 1 : 0;
            ins88[3] = keynr;
            ins88[4] = curEcm88len;
            //
            // we should check the nano to make sure the ecm is valid
            // we should look for at least 1 E3 nano, 1 EA nano and the F0 signature nano
            //
            // DE04
            if (DE04[0] == 0xDE)
            {
                uint32_t l = curEcm88len - 6;
                if (l > 256 || curEcm88len <= 6)   //don't known if this is ok...
                {
                    rdr_log(reader, "ecm invalid/too long! len=%d", curEcm88len);
                    return ERROR;
                }
                memcpy(DE04 + 6, (uchar *)ecm88Data, l);
                write_cmd(ins88, DE04); // request dcw
            }
            else
            {
                write_cmd(ins88, (uchar *)ecm88Data); // request dcw
            }
            //
            write_cmd(insc0, NULL); // read dcw
            switch (cta_res[0])
            {
            case 0xe8: // even
                if (cta_res[1] == 8)
                {
                    memcpy(ea->cw, cta_res + 2, 8);
                    rc = 1;
                }
                break;
            case 0xe9: // odd
                if (cta_res[1] == 8)
                {
                    memcpy(ea->cw + 8, cta_res + 2, 8);
                    rc = 1;
                }
                break;
            case 0xea: // complete
                if (cta_res[1] == 16)
                {
                    memcpy(ea->cw, cta_res + 2, 16);
                    rc = 1;
                }
                break;
            default :
                ecm88Data = nextEcm;
                ecm88Len -= curEcm88len;
                rdr_debug_mask(reader, D_READER, "ECM: key to use is not the current one, trying next ECM");
                snprintf( ea->msglog, MSGLOGSIZE, "key to use is not the current one, trying next ECM" );
            }
        }
        else
        {
            //ecm88Data=nextEcm;
            //ecm88Len-=curEcm88len;
            rdr_debug_mask(reader, D_READER, "ECM: Unknown ECM type");
            snprintf( ea->msglog, MSGLOGSIZE, "Unknown ECM type" );
            return ERROR; /*Lets interupt the loop and exit, because we don't know this ECM type.*/
        }
    }

    if ( hasD2 && !dcw_crc(ea->cw) && (nanoD2 == 2 || nanoD2 == 4))
    {
        if (nanoD2 == 4)
        {
            hdSurEncPhase1(ea->cw);
            hdSurEncPhase2(ea->cw);
        }
        rdr_debug_mask(reader, D_READER, "Decoding CW : using AES key id %d for provider %06x", D2KeyID, (provid & 0xFFFFF0));
        rc = aes_decrypt_from_list(reader->aes_list, 0x500, (uint32_t) (provid & 0xFFFFF0), D2KeyID, ea->cw, 16);
        if ( rc == 0 )
            snprintf( ea->msglog, MSGLOGSIZE, "AES Decrypt : key id %d not found for CAID %04X , provider %06x", D2KeyID, 0x500, (provid & 0xFFFFF0) );
        if (nanoD2 == 4)
        {
            hdSurEncPhase1(ea->cw);
        }
    }

    return (rc ? OK : ERROR);
}

static int32_t viaccess_get_emm_type(EMM_PACKET *ep, struct s_reader *rdr)
{
    uint32_t provid = 0;
    rdr_debug_mask(rdr, D_EMM, "Entered viaccess_get_emm_type ep->emm[0]=%02x", ep->emm[0]);

    if (ep->emm[3] == 0x90 && ep->emm[4] == 0x03)
    {
        provid = ep->emm[5] << 16 | ep->emm[6] << 8 | (ep->emm[7] & 0xFE);
        i2b_buf(4, provid, ep->provid);
    }

    switch (ep->emm[0])
    {
    case 0x88:
        ep->type = UNIQUE;
        memset(ep->hexserial, 0, 8);
        memcpy(ep->hexserial, ep->emm + 4, 4);
        rdr_debug_mask(rdr, D_EMM, "UNIQUE");
        return (!memcmp(rdr->hexserial + 1, ep->hexserial, 4));

    case 0x8A:
    case 0x8B:
        ep->type = GLOBAL;
        rdr_debug_mask(rdr, D_EMM, "GLOBAL");
        return 1;

    case 0x8C:
    case 0x8D:
        ep->type = SHARED;
        rdr_debug_mask(rdr, D_EMM, "SHARED (part)");
        // We need those packets to pass otherwise we would never
        // be able to complete EMM reassembly
        return 1;

    case 0x8E:
        ep->type = SHARED;
        memset(ep->hexserial, 0, 8);
        memcpy(ep->hexserial, ep->emm + 3, 3);
        rdr_debug_mask(rdr, D_EMM, "SHARED");

        //check for provider as serial (cccam only?)
        int8_t i;
        for (i = 0; i < rdr->nprov; i++)
        {
            if (!memcmp(&rdr->prid[i][1], ep->hexserial, 3))
                return 1;
        }
        return (!memcmp(&rdr->sa[0][0], ep->hexserial, 3));

    default:
        ep->type = UNKNOWN;
        rdr_debug_mask(rdr, D_EMM, "UNKNOWN");
        return 1;
    }
}

static int32_t viaccess_get_emm_filter(struct s_reader *rdr, struct s_csystem_emm_filter **emm_filters, unsigned int *filter_count)
{
    if (*emm_filters == NULL)
    {
        const unsigned int max_filter_count = 4;
        if (!cs_malloc(emm_filters, max_filter_count * sizeof(struct s_csystem_emm_filter)))
            return ERROR;

        struct s_csystem_emm_filter *filters = *emm_filters;
        *filter_count = 0;

        int32_t idx = 0;

        filters[idx].type = EMM_SHARED;
        filters[idx].enabled   = 1;
        filters[idx].filter[0] = 0x8C;
        filters[idx].mask[0]   = 0xFF;
        idx++;

        filters[idx].type = EMM_SHARED;
        filters[idx].enabled   = 1;
        filters[idx].filter[0] = 0x8D;
        filters[idx].mask[0]   = 0xFF;
        idx++;

        filters[idx].type = EMM_SHARED;
        filters[idx].enabled   = 1;
        filters[idx].filter[0] = 0x8E;
        filters[idx].mask[0]   = 0xFF;
        memcpy(&filters[idx].filter[1], &rdr->sa[0][0], 3);
        memset(&filters[idx].mask[1], 0xFF, 3);
        idx++;

        filters[idx].type = EMM_UNIQUE;
        filters[idx].enabled   = 1;
        filters[idx].filter[0] = 0x88;
        filters[idx].mask[0]   = 0xFF;
        memcpy(&filters[idx].filter[1], rdr->hexserial + 1, 4);
        memset(&filters[idx].mask[1], 0xFF, 4);
        idx++;

        *filter_count = idx;
    }

    return OK;
}

static int32_t viaccess_do_emm(struct s_reader *reader, EMM_PACKET *ep)
{
    def_resp;
    static const unsigned char insa4[] = { 0xca, 0xa4, 0x04, 0x00, 0x03 }; // set provider id
    unsigned char insf0[] = { 0xca, 0xf0, 0x00, 0x01, 0x22 }; // set adf
    unsigned char insf4[] = { 0xca, 0xf4, 0x00, 0x01, 0x00 }; // set adf, encrypted
    unsigned char ins18[] = { 0xca, 0x18, 0x01, 0x01, 0x00 }; // set subscription
    unsigned char ins1c[] = { 0xca, 0x1c, 0x01, 0x01, 0x00 }; // set subscription, encrypted
    //static const unsigned char insc8[] = { 0xca,0xc8,0x00,0x00,0x02 }; // read extended status
    // static const unsigned char insc8Data[] = { 0x00,0x00 }; // data for read extended status
    struct viaccess_data *csystem_data = reader->csystem_data;

    int32_t emmdatastart = 7;


    if (ep->type == UNIQUE) emmdatastart++;
    int32_t emmLen = SCT_LEN(ep->emm) - emmdatastart;
    int32_t rc = 0;

    ///cs_dump(ep->emm, emmLen+emmdatastart, "RECEIVED EMM VIACCESS");

    int32_t emmUpToEnd;
    uchar *emmParsed = ep->emm + emmdatastart;
    int32_t provider_ok = 0;
    uint32_t emm_provid;
    uchar keynr = 0;
    int32_t ins18Len = 0;
    uchar ins18Data[512];
    uchar insData[512];
    uchar *nano81Data = 0;
    uchar *nano91Data = 0;
    uchar *nano92Data = 0;
    uchar *nano9EData = 0;
    uchar *nanoF0Data = 0;

    for (emmUpToEnd = emmLen; (emmParsed[1] != 0) && (emmUpToEnd > 0); emmUpToEnd -= (2 + emmParsed[1]), emmParsed += (2 + emmParsed[1]))
    {
        ///cs_dump (emmParsed, emmParsed[1] + 2, "NANO");

        if (emmParsed[0] == 0x90 && emmParsed[1] == 0x03)
        {
            /* identification of the service operator */

            uchar soid[3], ident[3], i;

            for (i = 0; i < 3; i++)
            {
                soid[i] = ident[i] = emmParsed[2 + i];
            }
            ident[2] &= 0xF0;
            emm_provid = b2i(3, ident);
            keynr = soid[2] & 0x0F;
            if (chk_prov(reader, ident, keynr))
            {
                provider_ok = 1;
            }
            else
            {
                rdr_log(reader, "EMM: provider or key not found on card (%x, %x)", emm_provid, keynr);
                return ERROR;
            }

            // check if the provider changes. If yes, set the new one. If not, don't .. card will return an error if we do.
            if ( csystem_data->last_geo.provid != emm_provid )
            {
                write_cmd(insa4, ident);
                if ( cta_res[cta_lr - 2] != 0x90 || cta_res[cta_lr - 1] != 0x00 )
                {
                    cs_dump(insa4, 5, "set provider cmd:");
                    cs_dump(soid, 3, "set provider data:");
                    rdr_log(reader, "update error: %02X %02X", cta_res[cta_lr - 2], cta_res[cta_lr - 1]);
                    return ERROR;
                }
            }
            // as we are maybe changing the used provider, clear the cache, so the next ecm will re-select the correct one
            csystem_data->last_geo.provid = 0;
            csystem_data->last_geo.geo_len = 0;
            csystem_data->last_geo.geo[0]  = 0;

        }
        else if (emmParsed[0] == 0x9e && emmParsed[1] == 0x20)
        {
            /* adf */

            if (!nano91Data)
            {
                /* adf is not crypted, so test it */

                uchar custwp;
                uchar *afd;

                custwp = reader->sa[0][3];
                afd = (uchar *)emmParsed + 2;

                if ( afd[31 - custwp / 8] & (1 << (custwp & 7)) )
                    rdr_debug_mask(reader, D_READER, "emm for our card %08X", b2i(4, &reader->sa[0][0]));
                else
                    return SKIPPED;
            }

            // memorize
            nano9EData = emmParsed;

        }
        else if (emmParsed[0] == 0x81)
        {
            nano81Data = emmParsed;
        }
        else if (emmParsed[0] == 0x91 && emmParsed[1] == 0x08)
        {
            nano91Data = emmParsed;
        }
        else if (emmParsed[0] == 0x92 && emmParsed[1] == 0x08)
        {
            nano92Data = emmParsed;
        }
        else if (emmParsed[0] == 0xF0 && emmParsed[1] == 0x08)
        {
            nanoF0Data = emmParsed;
        }
        else
        {
            /* other nanos */
            show_subs(reader, emmParsed);

            memcpy(ins18Data + ins18Len, emmParsed, emmParsed[1] + 2);
            ins18Len += emmParsed [1] + 2;
        }
    }

    if (!provider_ok)
    {
        rdr_debug_mask(reader, D_READER, "provider not found in emm, continue anyway");
        // force key to 1...
        keynr = 1;
        ///return ERROR;
    }

    if (!nanoF0Data)
    {
        cs_dump(ep->emm, ep->emmlen, "can't find 0xf0 in emm...");
        return ERROR; // error
    }

    if (nano9EData)
    {
        if (!nano91Data)
        {
            // set adf
            insf0[3] = keynr;  // key
            insf0[4] = nano9EData[1] + 2;
            write_cmd(insf0, nano9EData);
            if ( cta_res[cta_lr - 2] != 0x90 || cta_res[cta_lr - 1] != 0x00 )
            {
                cs_dump(insf0, 5, "set adf cmd:");
                cs_dump(nano9EData, insf0[4] , "set adf data:");
                rdr_log(reader, "update error: %02X %02X", cta_res[cta_lr - 2], cta_res[cta_lr - 1]);
                return ERROR;
            }
        }
        else
        {
            // set adf crypte
            insf4[3] = keynr;  // key
            insf4[4] = nano91Data[1] + 2 + nano9EData[1] + 2;
            memcpy (insData, nano91Data, nano91Data[1] + 2);
            memcpy (insData + nano91Data[1] + 2, nano9EData, nano9EData[1] + 2);
            write_cmd(insf4, insData);
            if (( cta_res[cta_lr - 2] != 0x90 && cta_res[cta_lr - 2] != 0x91) || cta_res[cta_lr - 1] != 0x00 )
            {
                cs_dump(insf4, 5, "set adf encrypted cmd:");
                cs_dump(insData, insf4[4], "set adf encrypted data:");
                rdr_log(reader, "update error: %02X %02X", cta_res[cta_lr - 2], cta_res[cta_lr - 1]);
                return ERROR;
            }
        }
    }

    if (!nano92Data)
    {
        // send subscription
        ins18[2] = nano9EData ? 0x01 : 0x00; // found 9E nano ?
        ins18[3] = keynr;  // key
        ins18[4] = ins18Len + nanoF0Data[1] + 2;
        memcpy (insData, ins18Data, ins18Len);
        memcpy (insData + ins18Len, nanoF0Data, nanoF0Data[1] + 2);
        write_cmd(ins18, insData);
        if ( (cta_res[cta_lr - 2] == 0x90 || cta_res[cta_lr - 2] == 0x91) && cta_res[cta_lr - 1] == 0x00 )
        {
            rdr_debug_mask(reader, D_READER, "update successfully written");
            rc = 1; // written
        }
        else
        {
            cs_dump(ins18, 5, "set subscription cmd:");
            cs_dump(insData, ins18[4], "set subscription data:");
            rdr_log(reader, "update error: %02X %02X", cta_res[cta_lr - 2], cta_res[cta_lr - 1]);
        }

    }
    else
    {
        // send subscription encrypted

        if (!nano81Data)
        {
            cs_dump(ep->emm, ep->emmlen, "0x92 found, but can't find 0x81 in emm...");
            return ERROR; // error
        }

        ins1c[2] = nano9EData ? 0x01 : 0x00; // found 9E nano ?
        if (ep->type == UNIQUE) ins1c[2] = 0x02;
        ins1c[3] = keynr;  // key
        ins1c[4] = nano92Data[1] + 2 + nano81Data[1] + 2 + nanoF0Data[1] + 2;
        memcpy (insData, nano92Data, nano92Data[1] + 2);
        memcpy (insData + nano92Data[1] + 2, nano81Data, nano81Data[1] + 2);
        memcpy (insData + nano92Data[1] + 2 + nano81Data[1] + 2, nanoF0Data, nanoF0Data[1] + 2);
        write_cmd(ins1c, insData);

        if ( (cta_res[cta_lr - 2] == 0x90 || cta_res[cta_lr - 2] == 0x91) &&
                (cta_res[cta_lr - 1] == 0x00 || cta_res[cta_lr - 1] == 0x08) )
        {
            rdr_log(reader, "update successfully written");
            rc = 1; // written
        }
        rc = 1;
        /* don't return ERROR at this place
        else {
            if( cta_res[cta_lr-2]&0x1 )
                rdr_log(reader, "update not written. Data already exists or unknown address");

            //if( cta_res[cta_lr-2]&0x8 ) {
            write_cmd(insc8, NULL);
            if( (cta_res[cta_lr-2]==0x90 && cta_res[cta_lr-1]==0x00) ) {
                rdr_log(reader, "extended status  %02X %02X", cta_res[0], cta_res[1]);
            }
            //}
            return ERROR;
        } */

    }

    /*
    Sub Main()
    Sc.Write("CA A4 04 00 03")
    RX
    Sc.Write("02 07 11")
    RX
    Sc.Write("CA F0 00 01 22")
    RX
    Sc.Write("9E 20")
    Sc.Write("10 10 08 8A 80 00 04 00 10 10 26 E8 54 80 1E 80")
    Sc.Write("00 01 00 00 00 00 00 50 00 00 80 02 22 00 08 50")
    RX
    Sc.Write("CA 18 01 01 11")
    RX
    Sc.Write("A9 05 34 DE 34 FF 80")
    Sc.Write("F0 08 1A 3E AF B5 2B EE E3 3B")
    RX

    End Sub
    */
    return rc;
}

static int32_t viaccess_card_info(struct s_reader *reader)
{
    def_resp;
    int32_t i, l;
    uchar insac[] = { 0xca, 0xac, 0x00, 0x00, 0x00 }; // select data
    uchar insb8[] = { 0xca, 0xb8, 0x00, 0x00, 0x00 }; // read selected data
    uchar insa4[] = { 0xca, 0xa4, 0x00, 0x00, 0x00 }; // select issuer
    uchar insc0[] = { 0xca, 0xc0, 0x00, 0x00, 0x00 }; // read data item
    static const uchar ins24[] = { 0xca, 0x24, 0x00, 0x00, 0x09 }; // set pin

    static const uchar cls[] = { 0x00, 0x21, 0xff, 0x9f};
    static const uchar pin[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};
    struct viaccess_data *csystem_data = reader->csystem_data;

    csystem_data->last_geo.provid  = 0;
    csystem_data->last_geo.geo_len = 0;
    csystem_data->last_geo.geo[0]  = 0;

    rdr_log(reader, "card detected");

    cs_clear_entitlement(reader); //reset the entitlements

    // set pin
    write_cmd(ins24, pin);

    insac[2] = 0xa4; write_cmd(insac, NULL); // request unique id
    insb8[4] = 0x07; write_cmd(insb8, NULL); // read unique id
    rdr_log_sensitive(reader, "serial: {%llu}", (unsigned long long) b2ll(5, cta_res + 2));

    insa4[2] = 0x00; write_cmd(insa4, NULL); // select issuer 0
    for (i = 1; (cta_res[cta_lr - 2] == 0x90) && (cta_res[cta_lr - 1] == 0); i++)
    {
        uint32_t l_provid, l_sa;
        uchar l_name[64];
        insc0[4] = 0x1a; write_cmd(insc0, NULL); // show provider properties
        cta_res[2] &= 0xF0;
        l_provid = b2i(3, cta_res);

        insac[2] = 0xa5; write_cmd(insac, NULL); // request sa
        insb8[4] = 0x06; write_cmd(insb8, NULL); // read sa
        l_sa = b2i(4, cta_res + 2);

        insac[2] = 0xa7; write_cmd(insac, NULL); // request name
        insb8[4] = 0x02; write_cmd(insb8, NULL); // read name nano + len
        l = cta_res[1];
        insb8[4] = l; write_cmd(insb8, NULL); // read name
        cta_res[l] = 0;
        trim((char *)cta_res);
        if (cta_res[0])
            snprintf((char *)l_name, sizeof(l_name), ", name: %s", cta_res);
        else
            l_name[0] = 0;

        // read GEO
        insac[2] = 0xa6; write_cmd(insac, NULL); // request GEO
        insb8[4] = 0x02; write_cmd(insb8, NULL); // read GEO nano + len
        l = cta_res[1];
        char tmp[l * 3 + 1];
        insb8[4] = l; write_cmd(insb8, NULL); // read geo
        rdr_log_sensitive(reader, "provider: %d, id: {%06X%s}, sa: {%08X}, geo: %s",
                          i, l_provid, l_name, l_sa, (l < 4) ? "empty" : cs_hexdump(1, cta_res, l, tmp, sizeof(tmp)));

        // read classes subscription
        insac[2] = 0xa9; insac[4] = 4;
        write_cmd(insac, cls); // request class subs
        while ( (cta_res[cta_lr - 2] == 0x90) && (cta_res[cta_lr - 1] == 0) )
        {
            insb8[4] = 0x02; write_cmd(insb8, NULL); // read class subs nano + len
            if ( (cta_res[cta_lr - 2] == 0x90) && (cta_res[cta_lr - 1] == 0) )
            {
                l = cta_res[1];
                insb8[4] = l; write_cmd(insb8, NULL); // read class subs
                if ( (cta_res[cta_lr - 2] == 0x90) &&
                        (cta_res[cta_lr - 1] == 0x00 || cta_res[cta_lr - 1] == 0x08) )
                {
                    show_class(reader, NULL, l_provid, cta_res, cta_lr - 2);
                }
            }
        }

        insac[4] = 0;
        insa4[2] = 0x02;
        write_cmd(insa4, NULL); // select next provider
    }
    //return ERROR;
    return OK;
}

static int32_t viaccess_reassemble_emm(struct s_client *client, EMM_PACKET *ep)
{
    uint8_t *buffer = ep->emm;
    int16_t *len = &ep->emmlen;
    int32_t pos = 0, i;
    int16_t k;

    // Viaccess
    if (*len > 500) return 0;

    switch (buffer[0])
    {
    case 0x8c:
    case 0x8d:
        // emm-s part 1
        if (!memcmp(client->via_rass_emm, buffer, *len))
            return 0;

        // copy first part of the emm-s
        memcpy(client->via_rass_emm, buffer, *len);
        client->via_rass_emmlen = *len;
        //cs_ddump_mask(D_READER, buffer, len, "viaccess global emm:");
        return 0;

    case 0x8e:
        // emm-s part 2
        if (!client->via_rass_emmlen) return 0;

        //extract nanos from emm-gh and emm-s
        uchar emmbuf[512];

        cs_debug_mask(D_EMM, "[viaccess] %s: start extracting nanos", __func__);
        //extract from emm-gh
        for (i = 3; i < client->via_rass_emmlen; i += client->via_rass_emm[i + 1] + 2)
        {
            //copy nano (length determined by i+1)
            memcpy(emmbuf + pos, client->via_rass_emm + i, client->via_rass_emm[i + 1] + 2);
            pos += client->via_rass_emm[i + 1] + 2;
        }

        if (buffer[2] == 0x2c)
        {
            //add 9E 20 nano + first 32 bytes of emm content
            memcpy(emmbuf + pos, "\x9E\x20", 2);
            memcpy(emmbuf + pos + 2, buffer + 7, 32);
            pos += 34;

            //add F0 08 nano + 8 subsequent bytes of emm content
            memcpy(emmbuf + pos, "\xF0\x08", 2);
            memcpy(emmbuf + pos + 2, buffer + 39, 8);
            pos += 10;
        }
        else
        {
            //extract from variable emm-s
            for (k = 7; k < (*len); k += buffer[k + 1] + 2)
            {
                //copy nano (length determined by k+1)
                memcpy(emmbuf + pos, buffer + k, buffer[k + 1] + 2);
                pos += buffer[k + 1] + 2;
            }
        }

        cs_ddump_mask(D_EMM, buffer, *len, "[viaccess] %s: %s emm-s", __func__, (buffer[2] == 0x2c) ? "fixed" : "variable");

        emm_sort_nanos(buffer + 7, emmbuf, pos);
        pos += 7;

        //calculate emm length and set it on position 2
        buffer[2] = pos - 3;

        cs_ddump_mask(D_EMM, client->via_rass_emm, client->via_rass_emmlen, "[viaccess] %s: emm-gh", __func__);
        cs_ddump_mask(D_EMM, buffer, pos, "[viaccess] %s: assembled emm", __func__);

        *len = pos;
        break;
    }
    return 1;
}

void reader_viaccess(struct s_cardsystem *ph)
{
    ph->do_emm_reassembly = viaccess_reassemble_emm;
    ph->do_emm = viaccess_do_emm;
    ph->do_ecm = viaccess_do_ecm;
    ph->card_info = viaccess_card_info;
    ph->card_init = viaccess_card_init;
    ph->get_emm_type = viaccess_get_emm_type;
    ph->get_emm_filter = viaccess_get_emm_filter;
    ph->caids[0] = 0x05;
    ph->desc = "viaccess";
}
#endif
