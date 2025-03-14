/*-
 * Copyright (c) 2009 Kai Wang
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS `AS IS' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "_libdwarf.h"

ELFTC_VCSID("$Id: dwarf_types.m4 2697 2012-11-24 17:12:36Z kaiwang27 $");

/* WARNING: GENERATED FROM dwarf_types.m4. */



int
dwarf_get_types(Dwarf_Debug dbg, Dwarf_Type **types,
    Dwarf_Signed *ret_count, Dwarf_Error *error)
{
	Dwarf_Section *ds;
	int ret;

	if (dbg == NULL || types == NULL || ret_count == NULL) {
		DWARF_SET_ERROR(dbg, error, DW_DLE_ARGUMENT);
		return (DW_DLV_ERROR);
	}

	if (dbg->dbg_types == NULL) {
		if ((ds = _dwarf_find_section(dbg, ".debug_typenames")) != NULL) {
			ret = _dwarf_nametbl_init(dbg, &dbg->dbg_types, ds,
			    error);
			if (ret != DW_DLE_NONE)
				return (DW_DLV_ERROR);
		}
		if (dbg->dbg_types == NULL) {
			DWARF_SET_ERROR(dbg, error, DW_DLE_NO_ENTRY);
			return (DW_DLV_NO_ENTRY);
		}
	}

	*types = dbg->dbg_types->ns_array;
	*ret_count = dbg->dbg_types->ns_len;

	return (DW_DLV_OK);
}

int
dwarf_typename(Dwarf_Type type, char **ret_name, Dwarf_Error *error)
{
	Dwarf_Debug dbg;

	dbg = type != NULL ? type->np_nt->nt_cu->cu_dbg : NULL;

	if (type == NULL || ret_name == NULL) {
		DWARF_SET_ERROR(dbg, error, DW_DLE_ARGUMENT);
		return (DW_DLV_ERROR);
	}

	*ret_name = type->np_name;

	return (DW_DLV_OK);
}

int
dwarf_type_die_offset(Dwarf_Type type, Dwarf_Off *ret_offset,
    Dwarf_Error *error)
{
	Dwarf_NameTbl nt;
	Dwarf_Debug dbg;

	dbg = type != NULL ? type->np_nt->nt_cu->cu_dbg : NULL;

	if (type == NULL || ret_offset == NULL) {
		DWARF_SET_ERROR(dbg, error, DW_DLE_ARGUMENT);
		return (DW_DLV_ERROR);
	}

	nt = type->np_nt;
	assert(nt != NULL);

	*ret_offset = nt->nt_cu_offset + type->np_offset;

	return (DW_DLV_OK);
}

int
dwarf_type_cu_offset(Dwarf_Type type, Dwarf_Off *ret_offset,
    Dwarf_Error *error)
{
	Dwarf_NameTbl nt;
	Dwarf_Debug dbg;

	dbg = type != NULL ? type->np_nt->nt_cu->cu_dbg : NULL;

	if (type == NULL || ret_offset == NULL) {
		DWARF_SET_ERROR(dbg, error, DW_DLE_ARGUMENT);
		return (DW_DLV_ERROR);
	}

	nt = type->np_nt;
	assert(nt != NULL);

	*ret_offset = nt->nt_cu_offset;

	return (DW_DLV_OK);
}

int
dwarf_type_name_offsets(Dwarf_Type type, char **ret_name, Dwarf_Off *die_offset,
    Dwarf_Off *cu_offset, Dwarf_Error *error)
{
	Dwarf_CU cu;
	Dwarf_Debug dbg;
	Dwarf_NameTbl nt;

	dbg = type != NULL ? type->np_nt->nt_cu->cu_dbg : NULL;

	if (type == NULL || ret_name == NULL || die_offset == NULL ||
	    cu_offset == NULL) {
		DWARF_SET_ERROR(dbg, error, DW_DLE_ARGUMENT);
		return (DW_DLV_ERROR);
	}

	nt = type->np_nt;
	assert(nt != NULL);

	cu = nt->nt_cu;
	assert(cu != NULL);

	*ret_name = type->np_name;
	*die_offset = nt->nt_cu_offset + type->np_offset;
	*cu_offset = cu->cu_1st_offset;

	return (DW_DLV_OK);
}

void
dwarf_types_dealloc(Dwarf_Debug dbg, Dwarf_Type *types, Dwarf_Signed count)
{

	(void) dbg;
	(void) types;
	(void) count;
}

