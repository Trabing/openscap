/*
 * Copyright 2009 Red Hat Inc., Durham, North Carolina.
 * All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors:
 *      Maros Barabas <mbarabas@redhat.com>
 *      Lukas Kuklinek <lkuklinek@redhat.com>
 */

#include "util.h"
#include "_error.h"
#include "oscap.h"
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>

struct oscap_import_source {
	oscap_stream_type_t type;
	char *encoding;
	char *name;
};
OSCAP_GETTER(const char *, oscap_import_source, name) OSCAP_GETTER(oscap_stream_type_t, oscap_import_source, type)

struct oscap_export_target {
	oscap_stream_type_t type;
	char *name;
	char *encoding;
	int indent;
	char *indent_string;
};
OSCAP_GETTER(const char *, oscap_export_target, name)
OSCAP_GETTER(const char *, oscap_export_target, encoding)
OSCAP_GETTER(int, oscap_export_target, indent)
OSCAP_GETTER(oscap_stream_type_t, oscap_export_target, type)
OSCAP_GETTER(const char *, oscap_export_target, indent_string)

struct oscap_import_source *oscap_import_source_new_file(const char *filename, const char *encoding)
{

	struct oscap_import_source *source = oscap_calloc(1, sizeof(struct oscap_import_source));
	if (source == NULL)
		return NULL;

	source->type = OSCAP_STREAM_FILE;
	if (filename == NULL) {
		oscap_seterr(OSCAP_EFAMILY_OSCAP, OSCAP_EINVARG, "filename argument can't point to NULL");
		return NULL;
	}

	source->encoding = oscap_strdup(encoding);
	source->name = oscap_strdup(filename);

	return source;
}

struct oscap_import_source *oscap_import_source_new_URL(const char *url, const char *encoding)
{

	struct oscap_import_source *source = oscap_calloc(1, sizeof(struct oscap_import_source));
	if (source == NULL)
		return NULL;

	source->type = OSCAP_STREAM_URL;
	if (url == NULL) {
		oscap_seterr(OSCAP_EFAMILY_OSCAP, OSCAP_EINVARG, "url argument can't point to NULL");
		return NULL;
	}

	source->encoding = oscap_strdup(encoding);
	source->name = oscap_strdup(url);

	return source;
}

void oscap_import_source_free(struct oscap_import_source *source)
{

	if (source == NULL)
		return;

	if (source->encoding != NULL)
		oscap_free(source->encoding);
	if (source->name != NULL)
		oscap_free(source->name);
	oscap_free(source);
}

struct oscap_export_target *oscap_export_target_new_file(const char *filename, const char *encoding)
{
	struct oscap_export_target *target;

	if (filename == NULL) {
		oscap_seterr(OSCAP_EFAMILY_OSCAP, OSCAP_EINVARG, "filename argument can't point to NULL");
		return NULL;
	}

	target = oscap_calloc(1, sizeof(struct oscap_export_target));
	if (target == NULL)
		return NULL;

	target->type = OSCAP_STREAM_FILE;

	if (encoding == NULL)
		target->encoding = oscap_strdup("UTF-8");
	else
		target->encoding = oscap_strdup(encoding);

	target->name = oscap_strdup(filename);
	/* default values */
	target->indent = 1;
	target->indent_string = oscap_strdup("    ");

	return target;
}

struct oscap_export_target *oscap_export_target_new_URL(const char *url, const char *encoding)
{

	struct oscap_export_target *target = oscap_calloc(1, sizeof(struct oscap_export_target));
	if (target == NULL)
		return NULL;

	target->type = OSCAP_STREAM_URL;

	if (url == NULL) {
		oscap_seterr(OSCAP_EFAMILY_OSCAP, OSCAP_EINVARG, "filename argument can't point to NULL");
		return NULL;
	}

	if (encoding == NULL)
		target->encoding = oscap_strdup("UTF-8");
	else
		target->encoding = oscap_strdup(encoding);

	target->name = oscap_strdup(url);
	/* default values */
	target->indent = 1;
	target->indent_string = oscap_strdup("    ");

	return target;
}

void oscap_export_target_free(struct oscap_export_target *target)
{

	if (target == NULL)
		return;

	if (target->name != NULL)
		oscap_free(target->name);
	if (target->encoding != NULL)
		oscap_free(target->encoding);
	if (target->indent_string != NULL)
		oscap_free(target->indent_string);
	oscap_free(target);
}

int oscap_string_to_enum(const struct oscap_string_map *map, const char *str)
{
	__attribute__nonnull__(map);

	while (map->string && (str == NULL || strcmp(map->string, str) != 0))
		++map;
	return map->value;
}

const char *oscap_enum_to_string(const struct oscap_string_map *map, int val)
{
	__attribute__nonnull__(map);

	while (map->string && map->value != val)
		++map;
	return map->string;
}

char *oscap_strdup(const char *str)
{

	char *m;

	if (str == NULL)
		return NULL;

	m = strdup(str);

	if ((m == NULL) && (strlen(m) > 0))
		oscap_seterr(OSCAP_EFAMILY_GLIBC, errno, "Memory allocation failed.");

	return m;
}

char *oscap_strsep(char **str, const char *delim)
{
	if (str == NULL || *str == NULL)
		return NULL;
	char *ret = *str;
	*str = strchr(*str, *delim);
	if (*str) {
		**str = '\0';
		(*str)++;
	}
	return ret;
}

static const size_t CPE_SPLIT_INIT_ALLOC = 8;

char **oscap_split(char *str, const char *delim)
{

	__attribute__nonnull__(str);

	char **stringp = &str;
	int alloc = CPE_SPLIT_INIT_ALLOC;
	char **fields = oscap_alloc(alloc * sizeof(char *));
	if (!fields)
		return NULL;

	int i = 0;
	while (*stringp) {
		if (i + 2 > alloc) {
			void *old = fields;
			alloc *= 2;
			fields = oscap_realloc(fields, alloc * sizeof(char *));
			if (fields == NULL) {
				oscap_free(old);
				return NULL;
			}
		}
		fields[i++] = oscap_strsep(stringp, delim);
	}
	fields[i] = NULL;

	return fields;
}


int oscap_strcmp(const char *s1, const char *s2)
{
	if (s1 == NULL) return INT_MIN;
	if (s2 == NULL) return INT_MAX;
	return strcmp(s1, s2);
}

bool oscap_streq(const char *s1, const char *s2)
{
	return (oscap_strcmp(s1, s2) == 0);
}

char *oscap_trim(char *str)
{
	int off, i;
	if (str == NULL)
		return NULL;
	for (i = 0; isspace(str[i]); ++i) ;
	off = i;
	while (str[i]) {
		str[i - off] = str[i];
		++i;
	}
	for (i -= off; isspace(str[--i]) && i >= 0;) ;
	str[++i] = '\0';
	return str;
}

char *oscap_vsprintf(const char *fmt, va_list ap)
{
    if (fmt == NULL) return NULL;

    char *ret = NULL;
    va_list args;
    va_copy(args, ap);

    char foo[2];
    int length = vsnprintf(foo, 1, fmt, ap);
    if (length < 0) goto cleanup;

    ret = oscap_alloc(sizeof(char) * (length + 1));
    vsprintf(ret, fmt, args);
    assert(ret[length] == '\0');

cleanup:
    va_end(args);
    return ret;
}

char *oscap_sprintf(const char *fmt, ...)
{
    char *ret = NULL;
    va_list ap;
    va_start(ap, fmt);
    ret = oscap_vsprintf(fmt, ap);
    va_end(ap);
    return ret;
}

