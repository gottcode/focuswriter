/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* enchant
 * Copyright (C) 2003,2004 Dom Lachowicz
 *               2006-2007 Harri Pitkänen <hatapitk@iki.fi>
 *               2006 Anssi Hannula <anssi.hannula@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02110-1301, USA.
 *
 * In addition, as a special exception, Dom Lachowicz
 * gives permission to link the code of this program with
 * non-LGPL Spelling Provider libraries (eg: a MSFT Office
 * spell checker backend) and distribute linked combinations including
 * the two.  You must obey the GNU Lesser General Public License in all
 * respects for all of the code used other than said providers.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <libvoikko/voikko.h>

#include "enchant.h"
#include "enchant-provider.h"

/**
 * Voikko is a Finnish spell checker. More information is available from:
 *
 * http://voikko.sourceforge.net/
 */
ENCHANT_PLUGIN_DECLARE("Voikko")

static int
voikko_dict_check (EnchantDict * me, const char *const word, size_t len)
{
	int result;
	struct VoikkoHandle *voikko_handle;

	voikko_handle = (struct VoikkoHandle *) me->user_data;
	result = voikkoSpellCstr(voikko_handle, word);
	if (result == VOIKKO_SPELL_FAILED)
		return 1;
	else if (result == VOIKKO_SPELL_OK)
		return 0;
	else
		return -1;
}

static char **
voikko_dict_suggest (EnchantDict * me, const char *const word,
			 size_t len, size_t * out_n_suggs)
{
	char **sugg_arr;
	struct VoikkoHandle *voikko_handle;

	voikko_handle = (struct VoikkoHandle *) me->user_data;
	sugg_arr = voikkoSuggestCstr(voikko_handle, word);
	if (sugg_arr == NULL)
		return NULL;
	for (*out_n_suggs = 0; sugg_arr[*out_n_suggs] != NULL; (*out_n_suggs)++);
	return sugg_arr;
}

static EnchantDict *
voikko_provider_request_dict (EnchantProvider * me, const char *const tag)
{
	EnchantDict *dict;
	const char * voikko_error;
	struct VoikkoHandle *voikko_handle;

	/* Only Finnish is supported at the moment */
	if (strncmp(tag, "fi_FI", 6) != 0 && strncmp(tag, "fi", 3) != 0)
		return NULL;

	voikko_handle = voikkoInit(&voikko_error, "fi_FI", enchant_broker_get_param(me->owner, "enchant.voikko.dictionary.path"));
	if (voikko_error) {
		enchant_provider_set_error(me, voikko_error);
		return NULL;
	}

	dict = g_new0 (EnchantDict, 1);
	dict->user_data = (void *)(long) voikko_handle;
	dict->check = voikko_dict_check;
	dict->suggest = voikko_dict_suggest;

	return dict;
}

static void
voikko_provider_dispose_dict (EnchantProvider * me, EnchantDict * dict)
{
	voikkoTerminate((struct VoikkoHandle *) dict->user_data);
	g_free (dict);
}

static int
voikko_provider_dictionary_exists (struct str_enchant_provider * me,
								   const char *const tag)
{
	const char * voikko_error;
	struct VoikkoHandle *voikko_handle;

	/* Only Finnish is supported */
	if (strncmp(tag, "fi", 3) != 0)
		return 0;

	/* Check that a dictionary is actually available */
	voikko_handle = voikkoInit(&voikko_error, "fi_FI", enchant_broker_get_param(me->owner, "enchant.voikko.dictionary.path"));
	if (voikko_handle) {
		voikkoTerminate(voikko_handle);
		return 1;
	}
	else return 0;
}


static char **
voikko_provider_list_dicts (EnchantProvider * me,
				size_t * out_n_dicts)
{
	char ** out_list = NULL;
	const char * voikko_error;
	struct VoikkoHandle *voikko_handle;
	*out_n_dicts = 0;

	voikko_handle = voikkoInit(&voikko_error, "fi_FI", enchant_broker_get_param(me->owner, "enchant.voikko.dictionary.path"));
	if (voikko_handle) {
		voikkoTerminate(voikko_handle);
		*out_n_dicts = 1;
		out_list = g_new0 (char *, *out_n_dicts + 1);
		out_list[0] = g_strdup("fi");
	}

	return out_list;
}

static void
voikko_provider_free_string_list (EnchantProvider * me, char **str_list)
{
	g_strfreev (str_list);
}

static void
voikko_provider_dispose (EnchantProvider * me)
{
	g_free (me);
}

static const char *
voikko_provider_identify (EnchantProvider * me)
{
	return "voikko";
}

static const char *
voikko_provider_describe (EnchantProvider * me)
{
	return "Voikko Provider";
}

#ifdef __cplusplus
extern "C" {
#endif

ENCHANT_MODULE_EXPORT (EnchantProvider *)
		 init_enchant_provider (void);

EnchantProvider *
init_enchant_provider (void)
{
	EnchantProvider *provider;

	provider = g_new0 (EnchantProvider, 1);
	provider->dispose = voikko_provider_dispose;
	provider->request_dict = voikko_provider_request_dict;
	provider->dispose_dict = voikko_provider_dispose_dict;
	provider->dictionary_exists = voikko_provider_dictionary_exists;
	provider->identify = voikko_provider_identify;
	provider->describe = voikko_provider_describe;
	provider->list_dicts = voikko_provider_list_dicts;
	provider->free_string_list = voikko_provider_free_string_list;

	return provider;
}

#ifdef __cplusplus
}
#endif
