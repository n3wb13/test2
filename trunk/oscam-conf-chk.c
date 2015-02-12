#define MODULE_LOG_PREFIX "config"

#include "globals.h"
#include "oscam-conf-chk.h"
#include "oscam-garbage.h"
#include "oscam-net.h"
#include "oscam-string.h"

void chk_iprange(char *value, struct s_ip **base)
{
	int32_t i = 0;
	char *ptr1, *ptr2, *saveptr1 = NULL;
	struct s_ip *fip, *lip, *cip;

	if(!cs_malloc(&cip, sizeof(struct s_ip)))
		{ return; }
	fip = cip;

	for(ptr1 = strtok_r(value, ",", &saveptr1); ptr1; ptr1 = strtok_r(NULL, ",", &saveptr1))
	{
		if(i == 0)
			{ ++i; }
		else
		{
			if(!cs_malloc(&cip, sizeof(struct s_ip)))
				{ break; }
			lip->next = cip;
		}

		if((ptr2 = strchr(trim(ptr1), '-')))
		{
			*ptr2++ = '\0';
			cs_inet_addr(trim(ptr1), &cip->ip[0]);
			cs_inet_addr(trim(ptr2), &cip->ip[1]);
		}
		else
		{
			cs_inet_addr(ptr1, &cip->ip[0]);
			IP_ASSIGN(cip->ip[1], cip->ip[0]);
		}
		lip = cip;
	}
	lip = *base;
	*base = fip;
	clear_sip(&lip);
}

void chk_caidtab(char *caidasc, CAIDTAB *ctab)
{
	int32_t i;
	char *ptr1, *ptr2, *ptr3, *saveptr1 = NULL;
	CAIDTAB newctab;
	memset(&newctab, 0, sizeof(CAIDTAB));
	for(i = 1; i < CS_MAXCAIDTAB; newctab.mask[i++] = 0xffff) { ; }

	for(i = 0, ptr1 = strtok_r(caidasc, ",", &saveptr1); (i < CS_MAXCAIDTAB) && (ptr1); ptr1 = strtok_r(NULL, ",", &saveptr1))
	{
		uint32_t caid, mask, cmap;
		if((ptr3 = strchr(trim(ptr1), ':')))
			{ * ptr3++ = '\0'; }
		else
			{ ptr3 = ""; }

		if((ptr2 = strchr(trim(ptr1), '&')))
			{ * ptr2++ = '\0'; }
		else
			{ ptr2 = ""; }

		if(((caid = a2i(ptr1, 2)) | (mask = a2i(ptr2, -2)) | (cmap = a2i(ptr3, 2))) < 0x10000)
		{
			newctab.caid[i] = caid;
			newctab.mask[i] = mask;
			newctab.cmap[i++] = cmap;
		}
	}
	memcpy(ctab, &newctab, sizeof(CAIDTAB));
}

void chk_caidvaluetab(char *lbrlt, CAIDVALUETAB *tab, int32_t minvalue)
{
	int32_t i;
	char *ptr1, *ptr2, *saveptr1 = NULL;
	CAIDVALUETAB newtab;
	memset(&newtab, 0, sizeof(CAIDVALUETAB));

	for(i = 0, ptr1 = strtok_r(lbrlt, ",", &saveptr1); (i < CS_MAX_CAIDVALUETAB) && (ptr1); ptr1 = strtok_r(NULL, ",", &saveptr1))
	{
		int32_t caid, value;

		if((ptr2 = strchr(trim(ptr1), ':')))
			{ * ptr2++ = '\0'; }
		else
			{ ptr2 = ""; }

		if(((caid = a2i(ptr1, 2)) < 0xFFFF) | ((value = atoi(ptr2)) < 10000))
		{
			newtab.caid[i] = caid;
			if(value < minvalue) { value = minvalue; }
			newtab.value[i] = value;
			newtab.n = ++i;
		}
	}
	memcpy(tab, &newtab, sizeof(CAIDVALUETAB));
}

void chk_cacheex_valuetab(char *lbrlt, CECSPVALUETAB *tab)
{
	//[caid][&mask][@provid][$servid][:awtime][:]dwtime
	int32_t i;
	char *ptr = NULL, *saveptr1 = NULL;
	CECSPVALUETAB newtab;
	memset(&newtab, 0, sizeof(CECSPVALUETAB));

	for(i = 0, ptr = strtok_r(lbrlt, ",", &saveptr1); (i < CS_MAXCAIDTAB) && (ptr); ptr = strtok_r(NULL, ",", &saveptr1), i++)
	{
		int32_t caid = -1, cmask = -1, srvid = -1;
		int32_t j, provid = -1;
		int16_t awtime = -1, dwtime = -1;
		char *ptr1 = NULL, *ptr2 = NULL, *ptr3 = NULL, *ptr4 = NULL, *ptr5 = NULL, *saveptr2 = NULL;

		if((ptr4 = strchr(trim(ptr), ':')))
		{
			//awtime & dwtime
			*ptr4++ = '\0';
			for(j = 0, ptr5 = strtok_r(ptr4, ":", &saveptr2); (j < 2) && ptr5; ptr5 = strtok_r(NULL, ":", &saveptr2), j++)
			{
				if(!j)
				{
					dwtime = atoi(ptr5);
				}
				if(j)
				{
					awtime = dwtime;
					dwtime = atoi(ptr5);
				}
			}
		}
		if((ptr3 = strchr(trim(ptr), '$')))
		{
			*ptr3++ = '\0';
			srvid = a2i(ptr3, 4);
		}
		if((ptr2 = strchr(trim(ptr), '@')))
		{
			*ptr2++ = '\0';
			provid = a2i(ptr2, 6);
		}
		if((ptr1 = strchr(ptr, '&')))
		{
			*ptr1++ = '\0';
			cmask = a2i(ptr1, -2);
		}
		if(!ptr1 && !ptr2 && !ptr3 && !ptr4)  //only dwtime
			{ dwtime = atoi(ptr); }
		else
			{ caid = a2i(ptr, 2); }
		if((i == 0 && (caid <= 0)) || (caid > 0))
		{
			newtab.caid[i] = caid;
			newtab.cmask[i] = cmask;
			newtab.prid[i] = provid;
			newtab.srvid[i] = srvid;
			newtab.awtime[i] = awtime;
			newtab.dwtime[i] = dwtime;
			newtab.n = i + 1;
		}

	}
	memcpy(tab, &newtab, sizeof(CECSPVALUETAB));
}


void chk_cacheex_cwcheck_valuetab(char *lbrlt, CWCHECKTAB *tab)
{
	//caid[&mask][@provid][$servid]:mode:counter
	int32_t i;
	char *ptr = NULL, *saveptr1 = NULL;
	CWCHECKTAB newtab;
	memset(&newtab, 0, sizeof(CWCHECKTAB));

	for(i = 0, ptr = strtok_r(lbrlt, ",", &saveptr1); (i < CS_MAXCAIDTAB) && (ptr); ptr = strtok_r(NULL, ",", &saveptr1), i++)
	{
		int32_t caid = -1, cmask = -1, provid = -1, srvid = -1;
		int16_t mode = -1, counter = -1;

		char *ptr1 = NULL, *ptr2 = NULL, *ptr3 = NULL, *ptr4 = NULL, *ptr5 = NULL, *saveptr2 = NULL;

		if((ptr4 = strchr(trim(ptr), ':')))
		{
			*ptr4++ = '\0';
			ptr5 = strtok_r(ptr4, ":", &saveptr2);
			if(ptr5) mode = atoi(ptr5);
			ptr5 = strtok_r(NULL, ":", &saveptr2);
			if(ptr5) counter = atoi(ptr5);
		}
		if((ptr3 = strchr(trim(ptr), '$')))
		{
			*ptr3++ = '\0';
			srvid = a2i(ptr3, 4);
		}
		if((ptr2 = strchr(trim(ptr), '@')))
		{
			*ptr2++ = '\0';
			provid = a2i(ptr2, 6);
		}
		if((ptr1 = strchr(ptr, '&')))
		{
			*ptr1++ = '\0';
			cmask = a2i(ptr1, -2);
		}

		caid = a2i(ptr, 2);

		if((i == 0 && (caid <= 0)) || (caid > 0))
		{
			newtab.caid[i] = caid;
			newtab.cmask[i] = cmask;
			newtab.prid[i] = provid;
			newtab.srvid[i] = srvid;
			newtab.mode[i] = mode;
			newtab.counter[i] = counter;
			newtab.n = i + 1;
		}

	}
	memcpy(tab, &newtab, sizeof(CWCHECKTAB));
}


void chk_cacheex_hitvaluetab(char *lbrlt, CECSPVALUETAB *tab)
{
	//[caid][&mask][@provid][$servid]
	int32_t i;
	char *ptr = NULL, *saveptr1 = NULL;
	CECSPVALUETAB newtab;
	memset(&newtab, 0, sizeof(CECSPVALUETAB));

	for(i = 0, ptr = strtok_r(lbrlt, ",", &saveptr1); (i < CS_MAXCAIDTAB) && (ptr); ptr = strtok_r(NULL, ",", &saveptr1), i++)
	{
		int32_t caid = -1, cmask = -1, srvid = -1;
		int32_t provid = -1;
		char *ptr1 = NULL, *ptr2 = NULL, *ptr3 = NULL;

		if((ptr3 = strchr(trim(ptr), '$')))
		{
			*ptr3++ = '\0';
			srvid = a2i(ptr3, 4);
		}
		if((ptr2 = strchr(trim(ptr), '@')))
		{
			*ptr2++ = '\0';
			provid = a2i(ptr2, 6);
		}
		if((ptr1 = strchr(ptr, '&')))
		{
			*ptr1++ = '\0';
			cmask = a2i(ptr1, -2);
		}
		caid = a2i(ptr, 2);
		if(caid > 0)
		{
			newtab.caid[i] = caid;
			newtab.cmask[i] = cmask;
			newtab.prid[i] = provid;
			newtab.srvid[i] = srvid;
			newtab.n = i + 1;
		}

	}
	memcpy(tab, &newtab, sizeof(CECSPVALUETAB));
}

void chk_tuntab(char *tunasc, TUNTAB *ttab)
{
	int32_t i;
	clear_tuntab(ttab);
	errno = 0;
	char *caid_ptr, *savecaid_ptr = NULL;
	for(i = 0, caid_ptr = strtok_r(tunasc, ",", &savecaid_ptr); (caid_ptr); caid_ptr = strtok_r(NULL, ",", &savecaid_ptr), i++)
	{
		TUNTAB_DATA d;
		char *srvid_ptr  = strchr(trim(caid_ptr), '.');
		char *caidto_ptr = strchr(trim(caid_ptr), ':');
		if (!srvid_ptr)
			continue;
		*srvid_ptr++ = '\0';
		if (caidto_ptr)
			*caidto_ptr++ = '\0';
		d.bt_caidfrom = a2i(caid_ptr, 2);
		d.bt_srvid    = a2i(srvid_ptr, 2);
		d.bt_caidto   = 0;
		if (caidto_ptr)
			d.bt_caidto = a2i(caidto_ptr, 2);
		if (errno == EINVAL)
			continue;
		if (d.bt_caidfrom | d.bt_srvid | d.bt_caidto)
			tuntab_add(ttab, &d);
	}
}

void chk_services(char *labels, SIDTABS *sidtabs)
{
	int32_t i;
	char *ptr, *saveptr1 = NULL;
	SIDTAB *sidtab;
	SIDTABBITS newsidok, newsidno;
	newsidok = newsidno = 0;
	for(ptr = strtok_r(labels, ",", &saveptr1); ptr; ptr = strtok_r(NULL, ",", &saveptr1))
	{
		for(trim(ptr), i = 0, sidtab = cfg.sidtab; sidtab; sidtab = sidtab->next, i++)
		{
			if(!strcmp(sidtab->label, ptr)) { newsidok |= ((SIDTABBITS)1 << i); }
			if((ptr[0] == '!') && (!strcmp(sidtab->label, ptr + 1))) { newsidno |= ((SIDTABBITS)1 << i); }
		}
	}
	sidtabs->ok = newsidok;
	sidtabs->no = newsidno;
}

void chk_ftab(char *zFilterAsc, FTAB *ftab, const char *zType, const char *zName, const char *zFiltName)
{
	int32_t i, j;
	char *ptr1, *ptr2, *ptr3, *saveptr1 = NULL;
	int32_t nfilts = 0;

	clear_ftab(ftab);
	// Count filters
	char *in_filter = cs_strdup(zFilterAsc);
	if (!in_filter)
		return;
	for(i = 0, ptr1 = strtok_r(in_filter, ";", &saveptr1); (ptr1); ptr1 = strtok_r(NULL, ";", &saveptr1), i++)
	{
		if((zFiltName && zFiltName[0] == 'c') && !strchr(trim(ptr1), ':'))
		{
			cs_log("PANIC: CAID field not found in CHID parameter!");
			free(in_filter);
			return;
		}
		nfilts++;
	}
	free(in_filter);

	// Pointer to filter values
	char **ptr;
	if (!cs_malloc(&ptr, nfilts * sizeof(char *)))
		return;

	FTAB newftab = { .nfilts = nfilts };
	if (!cs_malloc(&newftab.filts, newftab.nfilts * sizeof(*newftab.filts)))
	{
		free(ptr);
		return;
	}

	for(i = 0, ptr1 = strtok_r(zFilterAsc, ";", &saveptr1); (i < newftab.nfilts) && (ptr1); ptr1 = strtok_r(NULL, ";", &saveptr1), i++)
	{
		ptr[i] = ptr1;
		if((ptr2 = strchr(trim(ptr1), ':')))
		{
			*ptr2++ = '\0';
			newftab.filts[i].caid = (uint16_t)a2i(ptr1, 4);
			ptr[i] = ptr2;
		}
		else if(zFiltName && zFiltName[0] == 'c')
		{
			cs_log("PANIC: CAID field not found in CHID parameter!");
			free(ptr);
			return;
		}
		else if(zFiltName && (zFiltName[0] == 'f' || zFiltName[0] == 'l') )
		{
			newftab.filts[i].caid = (uint16_t)a2i(ptr1, 4);
			ptr[i] = NULL;
		}
	}

	if(newftab.nfilts)
	{
		cs_log_dbg(D_CLIENT, "%s '%s' %s filter(s):", zType, zName, zFiltName);
	}
	for(i = 0; i < newftab.nfilts; i++)
	{
		cs_log_dbg(D_CLIENT, "CAID #%d: %04X", i, newftab.filts[i].caid);
		if(zFiltName && (zFiltName[0] == 'f' || zFiltName[0] == 'l') && ptr[i] == NULL) { continue; }
		for(j = 0, ptr3 = strtok_r(ptr[i], ",", &saveptr1); (j < CS_MAXPROV) && (ptr3); ptr3 = strtok_r(NULL, ",", &saveptr1), j++)
		{
			newftab.filts[i].prids[j] = a2i(ptr3, 6);
			newftab.filts[i].nprids++;
			cs_log_dbg(D_CLIENT, "%s #%d: %06X", zFiltName, j, newftab.filts[i].prids[j]);
		}
	}

	free(ptr);
	*ftab = newftab;
}

void chk_cltab(char *classasc, CLASSTAB *clstab)
{
	int32_t i;
	char *ptr1, *saveptr1 = NULL;
	CLASSTAB newclstab;
	memset(&newclstab, 0, sizeof(newclstab));
	newclstab.an = newclstab.bn = 0;
	for(i = 0, ptr1 = strtok_r(classasc, ",", &saveptr1); (i < CS_MAXCAIDTAB) && (ptr1); ptr1 = strtok_r(NULL, ",", &saveptr1))
	{
		ptr1 = trim(ptr1);
		if(ptr1[0] == '!')
			{ newclstab.bclass[newclstab.bn++] = (uchar)a2i(ptr1 + 1, 2); }
		else
			{ newclstab.aclass[newclstab.an++] = (uchar)a2i(ptr1, 2); }
	}
	memcpy(clstab, &newclstab, sizeof(CLASSTAB));
}

void chk_port_tab(char *portasc, PTAB *ptab)
{
	int32_t i, j, nfilts, ifilt, iport;
	PTAB *newptab;
	char *ptr1, *ptr2, *ptr3, *saveptr1 = NULL;
	char *ptr[CS_MAXPORTS] = {0};
	int32_t port[CS_MAXPORTS] = {0};
	if(!cs_malloc(&newptab, sizeof(PTAB)))
		{ return; }

	for(nfilts = i = 0, ptr1 = strtok_r(portasc, ";", &saveptr1); (i < CS_MAXPORTS) && (ptr1); ptr1 = strtok_r(NULL, ";", &saveptr1), i++)
	{
		ptr[i] = ptr1;

		if(!newptab->ports[i].ncd && !cs_malloc(&newptab->ports[i].ncd, sizeof(struct ncd_port)))
			{ break; }

		if((ptr2 = strchr(trim(ptr1), '@')))
		{
			*ptr2++ = '\0';
			newptab->ports[i].s_port = atoi(ptr1);

			//checking for des key for port
			newptab->ports[i].ncd->ncd_key_is_set = false;
			if((ptr3 = strchr(trim(ptr1), '{')))
			{
				*ptr3++ = '\0';
				if(key_atob_l(ptr3, newptab->ports[i].ncd->ncd_key, sizeof(newptab->ports[i].ncd->ncd_key) * 2))
					{ fprintf(stderr, "newcamd: error in DES Key for port %s -> ignored\n", ptr1); }
				else
					{ newptab->ports[i].ncd->ncd_key_is_set = true; }
			}

			ptr[i] = ptr2;
			port[i] = newptab->ports[i].s_port;
			newptab->nports++;
		}
		nfilts++;
	}

	if(nfilts == 1 && strlen(portasc) < 6 && newptab->ports[0].s_port == 0)
	{
		newptab->ports[0].s_port = atoi(portasc);
		newptab->nports = 1;
	}

	iport = ifilt = 0;
	for(i = 0; i < nfilts; i++)
	{
		if(port[i] != 0)
			{ iport = i; }
		for(j = 0, ptr3 = strtok_r(ptr[i], ",", &saveptr1); (j < CS_MAXPROV) && (ptr3); ptr3 = strtok_r(NULL, ",", &saveptr1), j++)
		{
			if((ptr2 = strchr(trim(ptr3), ':')))
			{
				*ptr2++ = '\0';
				newptab->ports[iport].ncd->ncd_ftab.nfilts++;
				ifilt = newptab->ports[iport].ncd->ncd_ftab.nfilts - 1;
				newptab->ports[iport].ncd->ncd_ftab.filts[ifilt].caid = (uint16_t)a2i(ptr3, 4);
				newptab->ports[iport].ncd->ncd_ftab.filts[ifilt].prids[j] = a2i(ptr2, 6);
			}
			else
			{
				newptab->ports[iport].ncd->ncd_ftab.filts[ifilt].prids[j] = a2i(ptr3, 6);
			}
			newptab->ports[iport].ncd->ncd_ftab.filts[ifilt].nprids++;
		}
	}
	memcpy(ptab, newptab, sizeof(PTAB));
	NULLFREE(newptab);
}

void chk_ecm_whitelist(char *value, ECM_WHITELIST *ecm_whitelist)
{
	clear_ecm_whitelist(ecm_whitelist);
	char *ptr, *saveptr1 = NULL;
	for(ptr = strtok_r(value, ";", &saveptr1); ptr; ptr = strtok_r(NULL, ";", &saveptr1))
	{
		ECM_WHITELIST_DATA d;
		memset(&d, 0, sizeof(d));
		char *caid_end_ptr = strchr(ptr, ':'); // caid_end_ptr + 1 -> headers
		char *provid_ptr = strchr(ptr, '@'); // provid_ptr + 1 -> provid
		char *headers = ptr;
		if(caid_end_ptr)
		{
			caid_end_ptr[0] = '\0';
			if (provid_ptr)
			{
				provid_ptr[0] = '\0';
				provid_ptr++;
				d.ident = a2i(provid_ptr, 6);
			}
			d.caid = dyn_word_atob(ptr);
			headers = caid_end_ptr + 1; // -> headers
		} else if(provid_ptr) {
			provid_ptr[0] = '\0';
			d.ident = a2i(provid_ptr, 6);
		}
		if (d.caid == 0xffff) d.caid = 0;
		if (d.ident == 0xffff) d.ident = 0;
		char *len_ptr, *savelen_ptr = NULL;
		for(len_ptr = strtok_r(headers, ",", &savelen_ptr); len_ptr; len_ptr = strtok_r(NULL, ",", &savelen_ptr))
		{
			d.len = dyn_word_atob(len_ptr);
			if (d.len == 0xffff)
				continue;
			ecm_whitelist_add(ecm_whitelist, &d);
		}
	}
}

void chk_ecm_hdr_whitelist(char *value, ECM_HDR_WHITELIST *ecm_hdr_whitelist)
{
	clear_ecm_hdr_whitelist(ecm_hdr_whitelist);
	char *ptr, *saveptr = NULL;
	for(ptr = strtok_r(value, ";", &saveptr); ptr; ptr = strtok_r(NULL, ";", &saveptr))
	{
		ECM_HDR_WHITELIST_DATA d;
		memset(&d, 0, sizeof(d));
		char *caid_end_ptr = strchr(ptr, ':'); // caid_end_ptr + 1 -> headers
		char *provid_ptr = strchr(ptr, '@'); // provid_ptr + 1 -> provid
		char *headers = ptr;
		if(caid_end_ptr)
		{
			caid_end_ptr[0] = '\0';
			if (provid_ptr)
			{
				provid_ptr[0] = '\0';
				provid_ptr++;
				d.provid = a2i(provid_ptr, 6);
			}
			d.caid = dyn_word_atob(ptr);
			headers = caid_end_ptr + 1; // -> headers
		} else if(provid_ptr) {
			provid_ptr[0] = '\0';
			d.provid = a2i(provid_ptr, 6);
		}
		if (d.caid == 0xffff) d.caid = 0;
		if (d.provid == 0xffff) d.provid = 0;
		char *hdr_ptr, *savehdr_ptr = NULL;
		for(hdr_ptr = strtok_r(headers, ",", &savehdr_ptr); hdr_ptr; hdr_ptr = strtok_r(NULL, ",", &savehdr_ptr))
		{
			hdr_ptr = trim(hdr_ptr);
			d.len = strlen(hdr_ptr);
			if (d.len / 2 > sizeof(d.header))
				d.len = sizeof(d.header) * 2;
			if (d.len > 1)
			{
				key_atob_l(hdr_ptr, d.header, d.len);
				ecm_hdr_whitelist_add(ecm_hdr_whitelist, &d);
			}
		}
	}
}

/* Clears the s_ip structure provided. The pointer will be set to NULL so everything is cleared.*/
void clear_sip(struct s_ip **sip)
{
	struct s_ip *cip = *sip;
	for(*sip = NULL; cip != NULL; cip = cip->next)
	{
		add_garbage(cip);
	}
}

/* Clears the s_ptab struct provided by setting nfilts and nprids to zero. */
void clear_ptab(struct s_ptab *ptab)
{
	int32_t i;
	ptab->nports = 0;
	for(i = 0; i < CS_MAXPORTS; i++)
	{
		if(ptab->ports[i].ncd)
		{
			ptab->ports[i].ncd->ncd_ftab.nfilts = 0;
			ptab->ports[i].ncd->ncd_ftab.filts[0].nprids = 0;
			NULLFREE(ptab->ports[i].ncd);
			ptab->ports[i].ncd = NULL;
		}
	}
}

/* Clears given caidtab */
void clear_caidtab(struct s_caidtab *ctab)
{
	memset(ctab, 0, sizeof(struct s_caidtab));
	int32_t i;
	for(i = 1; i < CS_MAXCAIDTAB; ctab->mask[i++] = 0xffff) { ; }
}

/* Clears given csptab */
void clear_cacheextab(CECSPVALUETAB *ctab)
{
	memset(ctab, -1, sizeof(CECSPVALUETAB));
	ctab->n = 0;
}

static void array_clear(void **arr_data, int32_t *arr_num_entries)
{
	*arr_num_entries = 0;
	if (arr_data)
	{
		free(*arr_data);
		*arr_data = NULL;
	}
}

void clear_tuntab(struct s_tuntab *ttab)
{
	if (ttab) array_clear((void **)&ttab->ttdata, &ttab->ttnum);
}

void clear_ecm_whitelist(ECM_WHITELIST *ecm_whitelist)
{
	if (ecm_whitelist) array_clear((void **)&ecm_whitelist->ewdata, &ecm_whitelist->ewnum);
}

void clear_ecm_hdr_whitelist(ECM_HDR_WHITELIST *ecm_hdr_whitelist)
{
	if (ecm_hdr_whitelist) array_clear((void **)&ecm_hdr_whitelist->ehdata, &ecm_hdr_whitelist->ehnum);
}

void clear_ftab(struct s_ftab *ftab)
{
	if (ftab) array_clear((void **)ftab->filts, &ftab->nfilts);
}

/* Initializes dst_ttab with src_ttab data. If allocation fails clears dts_ttab */
void clone_ttab(TUNTAB *src_ttab, TUNTAB *dst_ttab)
{
	clear_tuntab(dst_ttab);
	if (src_ttab->ttdata)
	{
		if (!cs_malloc(&dst_ttab->ttdata, src_ttab->ttnum * sizeof(*src_ttab->ttdata)))
			return;
		memcpy(dst_ttab->ttdata, src_ttab->ttdata, src_ttab->ttnum * sizeof(*src_ttab->ttdata));
		dst_ttab->ttnum = src_ttab->ttnum;
	}
}

/* Initializes dst_ftab with src_ftab data. If allocation fails clears dts_ftab */
void clone_ftab(FTAB *src_ftab, FTAB *dst_ftab)
{
	clear_ftab(dst_ftab);
	if (src_ftab->filts)
	{
		if (!cs_malloc(&dst_ftab->filts, src_ftab->nfilts * sizeof(*src_ftab->filts)))
			return;
		memcpy(dst_ftab->filts, src_ftab->filts, src_ftab->nfilts * sizeof(*src_ftab->filts));
		dst_ftab->nfilts = src_ftab->nfilts;
	}
}

static bool array_add(void **arr_data, int32_t *arr_num_entries, uint32_t entry_size, void *new_entry)
{
	if (!cs_realloc(arr_data, (*arr_num_entries + 1) * entry_size))
		return false;
	memcpy(*arr_data + (*arr_num_entries * entry_size), new_entry, entry_size);
	*arr_num_entries += 1;
	return true;
}

void ecm_whitelist_add(ECM_WHITELIST *ecm_whitelist, ECM_WHITELIST_DATA *ew)
{
	array_add((void **)&ecm_whitelist->ewdata, &ecm_whitelist->ewnum, sizeof(*ecm_whitelist->ewdata), ew);
}

void ecm_hdr_whitelist_add(ECM_HDR_WHITELIST *ecm_hdr_whitelist, ECM_HDR_WHITELIST_DATA *eh)
{
	array_add((void **)&ecm_hdr_whitelist->ehdata, &ecm_hdr_whitelist->ehnum, sizeof(*ecm_hdr_whitelist->ehdata), eh);
}

void tuntab_add(TUNTAB *ttab, TUNTAB_DATA *td)
{
	array_add((void **)&ttab->ttdata, &ttab->ttnum, sizeof(*ttab->ttdata), td);
}

void ftab_add(FTAB *ftab, FILTER *filter)
{
	array_add((void **)&ftab->filts, &ftab->nfilts, sizeof(*ftab->filts), filter);
}
