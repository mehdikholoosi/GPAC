/*
 *			GPAC - Multimedia Framework C SDK
 *
 *			Copyright (c) Jean Le Feuvre 2000-2005 
 *					All rights reserved
 *
 *  This file is part of GPAC / BIFS codec sub-project
 *
 *  GPAC is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  GPAC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *   
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */



#include <gpac/internal/bifs_dev.h>
#include <gpac/internal/scenegraph_dev.h>
#include <gpac/internal/bifs_tables.h>
#include "quant.h" 
#include "script.h" 


GF_Err gf_bifs_field_index_by_mode(GF_Node *node, u32 all_ind, u8 indexMode, u32 *outField)
{
	GF_Err e;
	u32 i, count, temp;
	count = gf_node_get_num_fields_in_mode(node, indexMode);
	for (i=0; i<count; i++) {
		e = gf_bifs_get_field_index(node, i, indexMode, &temp);
		if (e) return e;
		if (temp==all_ind) {
			*outField = i;
			return GF_OK;
		}
	}
	return GF_BAD_PARAM;
}

void BE_LogFloat(GF_BifsEncoder *codec, Fixed val, u32 nbBits, char *str, char *com);

void BE_WriteSFFloat(GF_BifsEncoder *codec, Fixed val, GF_BitStream *bs, char *com)
{
	if (codec->ActiveQP && codec->ActiveQP->useEfficientCoding) {
		gf_bifs_enc_mantissa_float(codec, val, bs);
	} else {
		gf_bs_write_float(bs, FIX2FLT(val));
		BE_LogFloat(codec, val, 32, "SFFloat", com);
	}
}


GF_Err gf_bifs_enc_sf_field(GF_BifsEncoder *codec, GF_BitStream *bs, GF_Node *node, GF_FieldInfo *field)
{
	GF_Err e;

	if (node) {
		e = gf_bifs_enc_quant_field(codec, bs, node, field);
		if (e != GF_EOS) return e;
	}
	switch (field->fieldType) {
	case GF_SG_VRML_SFBOOL:
		GF_BE_WRITE_INT(codec, bs, * ((SFBool *)field->far_ptr), 1, "SFBool", NULL);
		break;
	case GF_SG_VRML_SFCOLOR:
		BE_WriteSFFloat(codec, ((SFColor *)field->far_ptr)->red, bs, "color.red");
		BE_WriteSFFloat(codec, ((SFColor *)field->far_ptr)->green, bs, "color.green");
		BE_WriteSFFloat(codec, ((SFColor *)field->far_ptr)->blue, bs, "color.blue");
		break;
	case GF_SG_VRML_SFFLOAT:
		BE_WriteSFFloat(codec, * ((SFFloat *)field->far_ptr), bs, NULL);
		break;
	case GF_SG_VRML_SFINT32:
		GF_BE_WRITE_INT(codec, bs, * ((SFInt32 *)field->far_ptr), 32, "SFInt32", NULL);
		break;
	case GF_SG_VRML_SFROTATION:
		BE_WriteSFFloat(codec, ((SFRotation  *)field->far_ptr)->x, bs, "rot.x");
		BE_WriteSFFloat(codec, ((SFRotation  *)field->far_ptr)->y, bs, "rot.y");
		BE_WriteSFFloat(codec, ((SFRotation  *)field->far_ptr)->z, bs, "rot.z");
		BE_WriteSFFloat(codec, ((SFRotation  *)field->far_ptr)->q, bs, "rot.theta");
		break;

	case GF_SG_VRML_SFSTRING:
	{
		u32 i;
		char *str = ((SFString*)field->far_ptr)->buffer;
		u32 len = str ? strlen(str) : 0;
		u32 val = gf_get_bit_size(len);
		GF_BE_WRITE_INT(codec, bs, val, 5, "nbBits", NULL);
		GF_BE_WRITE_INT(codec, bs, len, val, "length", NULL);
		for (i=0; i<len; i++) gf_bs_write_int(bs, str[i], 8);
		if (codec->trace) fprintf(codec->trace, "string\t\t%d\t\t%s\n", 8*len, str);
	}
		break;

	case GF_SG_VRML_SFTIME:
		gf_bs_write_double(bs, *((SFTime *)field->far_ptr));
		if (codec->trace) fprintf(codec->trace, "SFTime\t\t%d\t\t%g\n", 64, *((SFTime *)field->far_ptr));
		break;

	case GF_SG_VRML_SFVEC2F:
		BE_WriteSFFloat(codec, ((SFVec2f *)field->far_ptr)->x, bs, "vec2f.x");
		BE_WriteSFFloat(codec, ((SFVec2f *)field->far_ptr)->y, bs, "vec2f.y");
		break;
	
	case GF_SG_VRML_SFVEC3F:
		BE_WriteSFFloat(codec, ((SFVec3f *)field->far_ptr)->x, bs, "vec3f.x");
		BE_WriteSFFloat(codec, ((SFVec3f *)field->far_ptr)->y, bs, "vec3f.y");
		BE_WriteSFFloat(codec, ((SFVec3f *)field->far_ptr)->z, bs, "vec3f.z");
		break;

	case GF_SG_VRML_SFURL:
	{
		SFURL *url = (SFURL *) field->far_ptr;
		GF_BE_WRITE_INT(codec, bs, (url->OD_ID>0) ? 1 : 0, 1, "hasODID", "SFURL");
		if (url->OD_ID>0) {
			GF_BE_WRITE_INT(codec, bs, url->OD_ID, 10, "ODID", "SFURL");
		} else {
			u32 i;
			u32 len = url->url ? strlen(url->url) : 0;
			u32 val = gf_get_bit_size(len);
			GF_BE_WRITE_INT(codec, bs, val, 5, "nbBits", NULL);
			GF_BE_WRITE_INT(codec, bs, len, val, "length", NULL);
			for (i=0; i<len; i++) gf_bs_write_int(bs, url->url[i], 8);
			if (codec->trace) fprintf(codec->trace, "string\t\t%d\t\t%s\t\t//SFURL\n", 8*len, url->url);
		}
	}
		break;
	case GF_SG_VRML_SFIMAGE:
	{
		u32 size, i;
		SFImage *img = (SFImage *)field->far_ptr;
		GF_BE_WRITE_INT(codec, bs, img->width, 12, "width", "SFImage");
		GF_BE_WRITE_INT(codec, bs, img->height, 12, "height", "SFImage");
		GF_BE_WRITE_INT(codec, bs, img->numComponents - 1, 2, "nbComp", "SFImage");
		size = img->width * img->height * img->numComponents;
		for (i=0; i<size; i++) gf_bs_write_int(bs, img->pixels[i], 8);
		if (codec->trace) fprintf(codec->trace, "pixels\t\t%d\t\tnot dumped\t\t//SFImage\n", 8*size);
	}
		break;

	case GF_SG_VRML_SFCOMMANDBUFFER:
	{
		SFCommandBuffer *cb = (SFCommandBuffer *) field->far_ptr;
		if (cb->buffer) free(cb->buffer);
		cb->buffer = NULL;
		cb->bufferSize = 0;
		if (gf_list_count(cb->commandList)) {
			u32 i, nbBits;
			GF_BitStream *bs_cond = gf_bs_new(NULL, 0, GF_BITSTREAM_WRITE);
			if (codec->trace) fprintf(codec->trace, "/*SFCommandBuffer*/\n");
			e = gf_bifs_enc_commands(codec, cb->commandList, bs_cond);
			if (!e) gf_bs_get_content(bs_cond, &cb->buffer, &cb->bufferSize);
			gf_bs_del(bs_cond);
			if (e) return e;
			if (codec->trace) fprintf(codec->trace, "/*End SFCommandBuffer*/\n");
			nbBits = gf_get_bit_size(cb->bufferSize);
			GF_BE_WRITE_INT(codec, bs, nbBits, 5, "NbBits", NULL);
			GF_BE_WRITE_INT(codec, bs, cb->bufferSize, nbBits, "BufferSize", NULL);
			for (i=0; i<cb->bufferSize; i++) GF_BE_WRITE_INT(codec, bs, cb->buffer[i], 8, "buffer byte", NULL);
		}
	}
		break;

	case GF_SG_VRML_SFNODE:
		return gf_bifs_enc_node(codec, *((GF_Node **)field->far_ptr), field->NDTtype, bs);

	case GF_SG_VRML_SFSCRIPT:
		codec->LastError = SFScript_Encode(codec, bs, node);
		break;
	default:
		return GF_NON_COMPLIANT_BITSTREAM;
	}
	return codec->LastError;
}


GF_Err gf_bifs_enc_mf_field(GF_BifsEncoder *codec, GF_BitStream *bs, GF_Node *node, GF_FieldInfo *field)
{
	GF_Node *child;
	GF_List *list = NULL;
	GF_Err e;
	u32 nbBits;
	Bool use_list;
	Bool qp_local, qp_on, initial_qp;
	u32 nbF, i;
	GF_FieldInfo sffield;
		
	nbF = 0;
	if (field->fieldType != GF_SG_VRML_MFNODE) {
		nbF = ((GenMFField *)field->far_ptr)->count;
	} else if (field->far_ptr) {
		list = *((GF_List **)field->far_ptr);
		nbF = gf_list_count(list);
	}
	/*reserved*/
	GF_BE_WRITE_INT(codec, bs, 0, 1, "reserved", NULL);
	if (!nbF) {
		/*is list*/
		GF_BE_WRITE_INT(codec, bs, 1, 1, "isList", NULL);
		/*end flag*/
		GF_BE_WRITE_INT(codec, bs, 1, 1, "end", NULL);
		return GF_OK;
	}

	/*do we work in list or vector*/
	use_list = 0;
	nbBits = gf_get_bit_size(nbF);
	if (nbBits + 5 > nbF + 1) use_list = 1;

	GF_BE_WRITE_INT(codec, bs, use_list, 1, "isList", NULL);
	if (!use_list) {
		GF_BE_WRITE_INT(codec, bs, nbBits, 5, "nbBits", NULL);
		GF_BE_WRITE_INT(codec, bs, nbF, nbBits, "length", NULL);
	}

	memset(&sffield, 0, sizeof(GF_FieldInfo));
	sffield.fieldIndex = field->fieldIndex;
	sffield.fieldType = gf_sg_vrml_get_sf_type(field->fieldType);
	sffield.NDTtype = field->NDTtype;

	initial_qp = qp_on = qp_local = 0;
	initial_qp = codec->ActiveQP ? 1 : 0;
	for (i=0; i<nbF; i++) {

		if (use_list) GF_BE_WRITE_INT(codec, bs, 0, 1, "end", NULL);

		if (field->fieldType != GF_SG_VRML_MFNODE) {
			gf_sg_vrml_mf_get_item(field->far_ptr, field->fieldType, &sffield.far_ptr, i);
			e = gf_bifs_enc_sf_field(codec, bs, node, &sffield);
		} else {
			child = gf_list_get(list, i);
			e = gf_bifs_enc_node(codec, child, field->NDTtype, bs);

			/*activate QP*/
			if (child->sgprivate->tag == TAG_MPEG4_QuantizationParameter) {
				qp_local = ((M_QuantizationParameter *)child)->isLocal;
				if (qp_on) gf_bifs_enc_qp_remove(codec, 0);
				e = gf_bifs_enc_qp_set(codec, child);
				if (e) return e;
				qp_on = 1;
				if (qp_local) qp_local = 2;
			}
		}
		
		if (e) return e;

		if (qp_on && qp_local) {
			if (qp_local == 2) qp_local -= 1;
			else {
				gf_bifs_enc_qp_remove(codec, initial_qp);
				qp_local = qp_on = 0;
			}
		}
	}

	if (use_list) GF_BE_WRITE_INT(codec, bs, 1, 1, "end", NULL);
	if (qp_on) gf_bifs_enc_qp_remove(codec, initial_qp);
	/*for QP14*/
	gf_bifs_enc_qp14_set_length(codec, nbF);
	return GF_OK;
}


GF_Err gf_bifs_enc_field(GF_BifsEncoder * codec, GF_BitStream *bs, GF_Node *node, GF_FieldInfo *field)
{
	assert(node);
	if (field->fieldType == GF_SG_VRML_UNKNOWN) 
		return GF_NON_COMPLIANT_BITSTREAM;
	
	if (gf_sg_vrml_is_sf_field(field->fieldType)) {
		return gf_bifs_enc_sf_field(codec, bs, node, field);
	}

	/*TO DO : PMF support*/

	if (codec->info->config.UsePredictiveMFField) {
		GF_BE_WRITE_INT(codec, bs, 0, 1, "usePredictive", NULL);
	}
	return gf_bifs_enc_mf_field(codec, bs, node, field);
}

/*we assume a node field is not ISed several times (that's stated as "undefined behaviour" in VRML*/
GF_Route *gf_bifs_enc_is_field_ised(GF_BifsEncoder *codec, GF_Node *node, u32 fieldIndex)
{
	GF_Route *r;
	u32 i;
	if (!codec->encoding_proto) return NULL;

	if (node->sgprivate->events) {
		i=0;
		while ((r = gf_list_enum(node->sgprivate->events, &i))) {
			if (!r->IS_route) continue;
			if ((r->ToNode == node) && (r->ToField.fieldIndex==fieldIndex)) return r;
			else if ((r->FromNode == node) && (r->FromField.fieldIndex==fieldIndex)) return r;
		}
	}

	i=0;
	while ((r = gf_list_enum(codec->encoding_proto->sub_graph->Routes, &i))) {
		if (!r->IS_route) continue;
		if ((r->ToNode == node) && (r->ToField.fieldIndex==fieldIndex)) return r;
		else if ((r->FromNode == node) && (r->FromField.fieldIndex==fieldIndex)) return r;
	}
	return NULL;
}

/**/
GF_Err EncNodeFields(GF_BifsEncoder * codec, GF_BitStream *bs, GF_Node *node)
{
	u8 mode;
	GF_Route *isedField;
	GF_Node *clone;
	GF_Err e;
	s32 *enc_fields;
	u32 numBitsALL, numBitsDEF, allInd, count, i, nbBitsProto, nbFinal;
	Bool use_list, nodeIsFDP = 0;
	GF_FieldInfo field, clone_field;


	e = GF_OK;

	if (codec->encoding_proto) {
		mode = GF_SG_FIELD_CODING_ALL;
		nbBitsProto = gf_get_bit_size(gf_sg_proto_get_field_count(codec->encoding_proto) - 1);
		numBitsALL = gf_get_bit_size(gf_node_get_num_fields_in_mode(node, GF_SG_FIELD_CODING_ALL) - 1);
	} else {
		mode = GF_SG_FIELD_CODING_DEF;
		nbBitsProto = 0;
		numBitsALL = 0;
	}
	count = gf_node_get_num_fields_in_mode(node, mode);
	if (node->sgprivate->tag==TAG_MPEG4_Script) count = 3;

	if (!count) {
		GF_BE_WRITE_INT(codec, bs, 0, 1, "isMask", NULL);
		GF_BE_WRITE_INT(codec, bs, 1, 1, "end", NULL);
		return GF_OK;
	}

	if (node->sgprivate->tag == TAG_ProtoNode) {
		clone = gf_sg_proto_create_instance(node->sgprivate->scenegraph, ((GF_ProtoInstance *)node)->proto_interface);;
	} else {
		clone = gf_node_new(node->sgprivate->scenegraph, node->sgprivate->tag);
	}
	if (clone) gf_node_register(clone, NULL);
	
	numBitsDEF = gf_get_bit_size(gf_node_get_num_fields_in_mode(node, GF_SG_FIELD_CODING_DEF) - 1);

	enc_fields = malloc(sizeof(s32) * count);
	nbFinal = 0;
	for (i=0; i<count; i++) {
		enc_fields[i] = -1;
		/*get field in ALL mode*/
		if (mode == GF_SG_FIELD_CODING_ALL) {
			allInd = i;
		} else {
			gf_bifs_get_field_index(node, i, GF_SG_FIELD_CODING_DEF, &allInd);
		}

		/*encode proto code*/
		if (codec->encoding_proto) {
			isedField = gf_bifs_enc_is_field_ised(codec, node, allInd);
			if (isedField) {
				enc_fields[i] = allInd;
				nbFinal ++;
				continue;
			}
		}
		/*common case*/
		gf_node_get_field(node, allInd, &field);
		/*if event don't encode (happens when encoding protos)*/
		if ((field.eventType == GF_SG_EVENT_IN) || (field.eventType == GF_SG_EVENT_OUT)) continue;
		/*if field is default skip*/
		switch (field.fieldType) {
		case GF_SG_VRML_SFNODE:
			if (* (GF_Node **) field.far_ptr) { enc_fields[i] = allInd; nbFinal++; }
			break;
		case GF_SG_VRML_MFNODE:
			if (gf_list_count(* (GF_List **) field.far_ptr) ) { enc_fields[i] = allInd; nbFinal++; }
			break;
		case GF_SG_VRML_SFCOMMANDBUFFER:
		{
			SFCommandBuffer *cb = (SFCommandBuffer *)field.far_ptr;
			if (gf_list_count(cb->commandList)) { enc_fields[i] = allInd; nbFinal++; }
		}
			break;
		default:
			gf_node_get_field(clone, allInd, &clone_field);
			if (!gf_sg_vrml_field_equal(clone_field.far_ptr, field.far_ptr, field.fieldType)) { enc_fields[i] = allInd; nbFinal++; }
			break;
		}
	}
	if (clone) gf_node_unregister(clone, NULL);

	use_list = 1;
	/* patch for FDP node : */
	/* cannot use default field sorting due to spec "mistake", so use list to imply inversion between field 2 and field 3 of FDP*/
	if (node->sgprivate->tag == TAG_MPEG4_FDP) {
		s32 s4SwapValue = enc_fields[2];
		enc_fields[2] = enc_fields[3];
		enc_fields[3] = s4SwapValue;
		nodeIsFDP = 1;
		use_list = 1;
	}
	/*number of bits in mask node is count*1, in list node is 1+nbFinal*(1+numBitsDEF) */
	else if (count < 1+nbFinal*(1+numBitsDEF)) 
		use_list = 0;

	GF_BE_WRITE_INT(codec, bs, use_list ? 0 : 1, 1, "isMask", NULL);

	for (i=0; i<count; i++) {
		if (enc_fields[i] == -1) {
			if (!use_list) GF_BE_WRITE_INT(codec, bs, 0, 1, "Mask", NULL);
			continue;
		}
		allInd = (u32) enc_fields[i];

		/*encode proto code*/
		if (codec->encoding_proto) {
			isedField = gf_bifs_enc_is_field_ised(codec, node, allInd);
			if (isedField) {
				if (use_list) {
					GF_BE_WRITE_INT(codec, bs, 0, 1, "end", NULL);
				} else {
					GF_BE_WRITE_INT(codec, bs, 1, 1, "Mask", NULL);
				}
				GF_BE_WRITE_INT(codec, bs, 1, 1, "isedField", NULL);
				if (use_list) GF_BE_WRITE_INT(codec, bs, allInd, numBitsALL, "nodeField", NULL);

				if (isedField->ToNode == node) {
					GF_BE_WRITE_INT(codec, bs, isedField->FromField.fieldIndex, nbBitsProto, "protoField", NULL);
				} else {
					GF_BE_WRITE_INT(codec, bs, isedField->ToField.fieldIndex, nbBitsProto, "protoField", NULL);
				}
				continue;
			}
		}
		/*common case*/
		gf_node_get_field(node, allInd, &field);
		if (use_list) {
			/*not end flag*/
			GF_BE_WRITE_INT(codec, bs, 0, 1, "end", NULL);
		} else {
			/*mask flag*/
			GF_BE_WRITE_INT(codec, bs, 1, 1, "Mask", NULL);
		}
		/*not ISed field*/
		if (codec->encoding_proto) GF_BE_WRITE_INT(codec, bs, 0, 1, "isedField", NULL);
		if (use_list) {
			if (codec->encoding_proto || nodeIsFDP) {
				u32 ind=0;
				/*for proto, we're in ALL mode and we need DEF mode*/
				/*for FDP, encoding requires to get def id from all id as fields 2 and 3 are reversed*/
				gf_bifs_field_index_by_mode(node, allInd, GF_SG_FIELD_CODING_DEF, &ind);
				GF_BE_WRITE_INT(codec, bs, ind, numBitsDEF, "field", (char*)field.name);
			} else {
				GF_BE_WRITE_INT(codec, bs, i, numBitsDEF, "field", (char*)field.name);
			}
		}
		e = gf_bifs_enc_field(codec, bs, node, &field);
		if (e) goto exit;
	}
	/*end flag*/
	if (use_list) GF_BE_WRITE_INT(codec, bs, 1, 1, "end", NULL);
exit:
	free(enc_fields);
	return e;
}

Bool BE_NodeIsUSE(GF_BifsEncoder * codec, GF_Node *node)
{
	u32 i, count;
	if (!node || !node->sgprivate->NodeID) return 0;
	count = gf_list_count(codec->encoded_nodes);
	for (i=0; i<count; i++) {
		if (gf_list_get(codec->encoded_nodes, i) == node) return 1;
	}
	gf_list_add(codec->encoded_nodes, node);
	return 0;
}

GF_Err gf_bifs_enc_node(GF_BifsEncoder * codec, GF_Node *node, u32 NDT_Tag, GF_BitStream *bs)
{
	u32 NDTBits, node_type, node_tag, BVersion;
	Bool flag;
	GF_Node *new_node;
	GF_Err e;

	assert(codec->info);

	/*NULL node is a USE of maxID*/
	if (!node) {
		GF_BE_WRITE_INT(codec, bs, 1, 1, "USE", NULL);
		GF_BE_WRITE_INT(codec, bs, (1<<codec->info->config.NodeIDBits) - 1 , codec->info->config.NodeIDBits, "NodeID", "NULL");
		return GF_OK;
	}

	flag = BE_NodeIsUSE(codec, node);
	GF_BE_WRITE_INT(codec, bs, flag ? 1 : 0, 1, "USE", (char*)gf_node_get_class_name(node));

	if (flag) {
		gf_bs_write_int(bs, node->sgprivate->NodeID - 1, codec->info->config.NodeIDBits);
		new_node = gf_bifs_enc_find_node(codec, node->sgprivate->NodeID);
		if (!new_node) return codec->LastError = GF_SG_UNKNOWN_NODE;
		
		/*restore QP14 length*/
		switch (gf_node_get_tag(new_node)) {
		case TAG_MPEG4_Coordinate:
		{
			u32 nbCoord = ((M_Coordinate *)new_node)->point.count;
			gf_bifs_enc_qp14_enter(codec, 1);
			gf_bifs_enc_qp14_set_length(codec, nbCoord);
			gf_bifs_enc_qp14_enter(codec, 0);
		}
			break;
		case TAG_MPEG4_Coordinate2D:
		{
			u32 nbCoord = ((M_Coordinate2D *)new_node)->point.count;
			gf_bifs_enc_qp14_enter(codec, 1);
			gf_bifs_enc_qp14_set_length(codec, nbCoord);
			gf_bifs_enc_qp14_enter(codec, 0);
		}
			break;
		}
		return GF_OK;
	}

	BVersion = GF_BIFS_V1;
	node_tag = node->sgprivate->tag;
	while (1) {
		node_type = gf_bifs_get_node_type(NDT_Tag, node_tag, BVersion);
		NDTBits = gf_bifs_get_ndt_bits(NDT_Tag, BVersion);
		if (BVersion==2 && (node_tag==TAG_ProtoNode)) node_type = 1;
		GF_BE_WRITE_INT(codec, bs, node_type, NDTBits, "ndt", NULL);
		if (node_type) break;

		BVersion += 1;
		if (BVersion > GF_BIFS_NUM_VERSION) return codec->LastError = GF_BIFS_UNKNOWN_VERSION;
	}
	if (BVersion==2 && node_type==1) {
		GF_Proto *proto = ((GF_ProtoInstance *)node)->proto_interface;
		GF_BE_WRITE_INT(codec, bs, proto->ID, codec->info->config.ProtoIDBits, "protoID", NULL);
	}

	/*special handling of 3D mesh*/

	/*DEF'd node*/
	GF_BE_WRITE_INT(codec, bs, node->sgprivate->NodeID ? 1 : 0, 1, "DEF", NULL);
	if (node->sgprivate->NodeID) {
		GF_BE_WRITE_INT(codec, bs, node->sgprivate->NodeID - 1, codec->info->config.NodeIDBits, "NodeID", NULL);
		if (codec->info->UseName) gf_bifs_enc_name(codec, bs, node->sgprivate->NodeName);
	}

	/*no updates of time fields for now - NEEDED FOR A LIVE ENCODER*/

	/*QP14 case*/
	switch (node_tag) {
	case TAG_MPEG4_Coordinate:
	case TAG_MPEG4_Coordinate2D:
		gf_bifs_enc_qp14_enter(codec, 1);
	}

	e = EncNodeFields(codec, bs, node);
	if (e) return e;

	switch (node_tag) {
	case TAG_MPEG4_IndexedFaceSet:
	case TAG_MPEG4_IndexedFaceSet2D:
	case TAG_MPEG4_IndexedLineSet:
	case TAG_MPEG4_IndexedLineSet2D:
		gf_bifs_enc_qp14_reset(codec);
		break;
	case TAG_MPEG4_Coordinate:
	case TAG_MPEG4_Coordinate2D:
		gf_bifs_enc_qp14_enter(codec, 0);
		break;
	}
	return GF_OK;
}


