/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/*
 * This file is part of The Croco Library
 *
 * Copyright (C) 2002-2003 Dodji Seketeli <dodji@seketeli.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2.1 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

/*
 *$Id$
 */

#include <string.h>
#include "cr-statement.h"
#include "cr-parser.h"

/**
 *@file
 *Definition of the #CRStatement class.
 */

#define DECLARATION_INDENT_NB 2

static void
cr_statement_clear (CRStatement *a_this) ;

static void
cr_statement_dump_ruleset (CRStatement *a_this, FILE *a_fp, glong a_indent) ;

static void
cr_statement_dump_charset (CRStatement *a_this, FILE *a_fp,
			   gulong a_indent) ;

static void
cr_statement_dump_page (CRStatement *a_this, FILE *a_fp, gulong a_indent) ;

static void
cr_statement_dump_media_rule (CRStatement *a_this, FILE *a_fp,
			      gulong a_indent) ;

static void
cr_statement_dump_import_rule (CRStatement *a_this, FILE *a_fp,
			       gulong a_indent) ;

static void
parse_font_face_start_font_face_cb (CRDocHandler *a_this)
{
	CRStatement *stmt = NULL ;
	enum CRStatus status = CR_OK ;

	stmt = cr_statement_new_at_font_face_rule (NULL,
						   NULL) ;
	g_return_if_fail (stmt) ;
	
	status = cr_doc_handler_set_ctxt (a_this, stmt) ;
	g_return_if_fail (status == CR_OK) ;
}

static void
parse_font_face_unrecoverable_error_cb (CRDocHandler *a_this)
{
        CRStatement *stmt = NULL ;
        enum CRStatus status = CR_OK ;

        g_return_if_fail (a_this) ;
        
        status = cr_doc_handler_get_ctxt (a_this, (gpointer*)&stmt) ;
        if (status != CR_OK)
        {
                cr_utils_trace_info ("Couldn't get parsing context. "
                                     "This may lead to some memory leaks.") ;
                return ;
        }
        if (stmt)
        {
                cr_statement_destroy (stmt) ;
                cr_doc_handler_set_ctxt (a_this, NULL) ;
                return ;
        }        
}

static void
parse_font_face_property_cb (CRDocHandler *a_this,
			     GString *a_name,
			     CRTerm *a_value)
{
	enum CRStatus status = CR_OK ;
	GString *name = NULL ;
	CRDeclaration *decl = NULL ;
	CRStatement *stmt = NULL ;

	g_return_if_fail (a_this && a_name) ;

	status = cr_doc_handler_get_ctxt (a_this, (gpointer*)&stmt) ;
	g_return_if_fail (status == CR_OK && stmt) ;
	g_return_if_fail (stmt->type == AT_FONT_FACE_RULE_STMT) ;

	name = g_string_new_len (a_name->str, a_name->len) ;
	g_return_if_fail (name) ;	
	decl = cr_declaration_new (stmt, name, a_value) ;
	if (!decl)
	{
		cr_utils_trace_info ("cr_declaration_new () failed.") ;
		goto error ;
	}
	name = NULL ;

	stmt->kind.font_face_rule->decl_list = 
		cr_declaration_append (stmt->kind.font_face_rule->decl_list,
				       decl) ;
	if (!stmt->kind.font_face_rule->decl_list)
		goto error ;
	decl = NULL ;

 error:
	if (decl)
	{
		cr_declaration_unref (decl) ;
		decl = NULL ;
	}
	if (name)
	{
		g_string_free (name, TRUE) ;
		name = NULL ;
	}
}

static void
parse_font_face_end_font_face_cb (CRDocHandler *a_this)

{	
	CRStatement *result = NULL ;
	enum CRStatus status = CR_OK ;
	
	g_return_if_fail (a_this) ;
	
	status = cr_doc_handler_get_ctxt (a_this, (gpointer*)&result) ;
	g_return_if_fail (status == CR_OK && result) ;
	g_return_if_fail (result->type == AT_FONT_FACE_RULE_STMT) ;

	status = cr_doc_handler_set_result (a_this, result) ;
	g_return_if_fail (status == CR_OK) ;
}

static void
parse_page_start_page_cb (CRDocHandler *a_this, 
			  GString *a_name,
			  GString *a_pseudo_page)
{
	CRStatement *stmt = NULL ;
	enum CRStatus status = CR_OK ;
	
	stmt = cr_statement_new_at_page_rule (NULL, NULL, a_name, 
					      a_pseudo_page) ;
	g_return_if_fail (stmt) ;
	status = cr_doc_handler_set_ctxt (a_this, stmt) ;
	g_return_if_fail (status == CR_OK) ;
}

static void
parse_page_unrecoverable_error_cb (CRDocHandler *a_this)
{
        CRStatement *stmt = NULL ;
        enum CRStatus status = CR_OK ;

        g_return_if_fail (a_this) ;

        status = cr_doc_handler_get_ctxt (a_this, (gpointer*)&stmt) ;
        if (status != CR_OK)
        {
                cr_utils_trace_info ("Couldn't get parsing context. "
                                     "This may lead to some memory leaks.") ;
                return ;
        }
        if (stmt)
        {
                cr_statement_destroy (stmt) ;
                stmt = NULL ;
                cr_doc_handler_set_ctxt (a_this, NULL) ;
        }
}

static void
parse_page_property_cb (CRDocHandler *a_this,
			GString *a_name, 
			CRTerm *a_expression)
{
	GString *name = NULL ;
	CRStatement *stmt = NULL ;
	CRDeclaration *decl = NULL ;
	enum CRStatus status = CR_OK ;

	status = cr_doc_handler_get_ctxt (a_this, (gpointer*)&stmt) ;
	g_return_if_fail (status == CR_OK && stmt->type == AT_PAGE_RULE_STMT) ;

	name = g_string_new_len (a_name->str, a_name->len) ;	
	g_return_if_fail (name) ;

	decl = cr_declaration_new (stmt, name, a_expression) ;
	g_return_if_fail (decl) ;

	stmt->kind.page_rule->decl_list = 
		cr_declaration_append (stmt->kind.page_rule->decl_list,
				       decl) ;
	g_return_if_fail (stmt->kind.page_rule->decl_list) ;
}

static void
parse_page_end_page_cb (CRDocHandler *a_this, 
			GString *a_name,
			GString *a_pseudo_page)
{
	enum CRStatus status = CR_OK ;
	CRStatement *stmt = NULL ;
	
	status = cr_doc_handler_get_ctxt (a_this, (gpointer*)&stmt) ;
	g_return_if_fail (status == CR_OK && stmt);
	g_return_if_fail (stmt->type == AT_PAGE_RULE_STMT);
        
	status = cr_doc_handler_set_result (a_this, stmt) ;
	g_return_if_fail (status == CR_OK) ;
}

static void
parse_at_media_start_media_cb (CRDocHandler *a_this,
			       GList *a_media_list)
{
	enum CRStatus status = CR_OK ;
	CRStatement *at_media = NULL ;
	GList *media_list = NULL ;

	g_return_if_fail (a_this && a_this->priv) ;

	if (a_media_list)
	{
		/*duplicate media list*/
		media_list = cr_dup_glist_of_string (a_media_list) ;
	}

	g_return_if_fail (media_list) ;

	/*make sure cr_statement_new_at_media_rule works in this case.*/
	at_media = cr_statement_new_at_media_rule (NULL, NULL,
						   media_list) ;

	status = cr_doc_handler_set_ctxt (a_this, at_media) ;        
	g_return_if_fail (status == CR_OK) ;
        status = cr_doc_handler_set_result (a_this, at_media) ;
        g_return_if_fail (status == CR_OK) ;
}

static void
parse_at_media_unrecoverable_error_cb (CRDocHandler *a_this)
{
        enum CRStatus status = CR_OK ;
        CRStatement * stmt = NULL ;

        g_return_if_fail (a_this) ;

        status = cr_doc_handler_get_result (a_this, (gpointer*)&stmt) ;
        if (status != CR_OK)
        {
                cr_utils_trace_info ("Couldn't get parsing context. "
                                     "This may lead to some memory leaks.") ;
                return ;
        }
        if (stmt)
        {
                cr_statement_destroy (stmt) ;
                stmt = NULL ;
                cr_doc_handler_set_ctxt (a_this, NULL) ;
                cr_doc_handler_set_result (a_this, NULL) ;
        }
}

static void
parse_at_media_start_selector_cb (CRDocHandler *a_this, 
				  CRSelector *a_sellist)
{
	enum CRStatus status = CR_OK ;
	CRStatement *at_media = NULL, *ruleset = NULL ;
	
	g_return_if_fail (a_this 
			  && a_this->priv
			  && a_sellist) ;
	
	status = cr_doc_handler_get_ctxt (a_this, 
					  (gpointer*)&at_media) ;
	g_return_if_fail (status == CR_OK && at_media) ;
	g_return_if_fail (at_media->type == AT_MEDIA_RULE_STMT) ;
	ruleset = cr_statement_new_ruleset (NULL, a_sellist,
					    NULL, at_media) ;
	g_return_if_fail (ruleset) ;
	status = cr_doc_handler_set_ctxt (a_this, ruleset) ;
	g_return_if_fail (status == CR_OK) ;
}

static void
parse_at_media_property_cb (CRDocHandler *a_this,
			    GString *a_name, CRTerm *a_value)
{
	enum CRStatus status = CR_OK ;
	/*
	 *the current ruleset stmt, child of the 
	 *current at-media being parsed.
	 */
	CRStatement *stmt = NULL;
	GString *name = NULL ;

	g_return_if_fail (a_this && a_name) ;
	
	name = g_string_new_len (a_name->str, a_name->len) ;
	g_return_if_fail (name) ;
	
	status = cr_doc_handler_get_ctxt (a_this, (gpointer*)&stmt) ;
	g_return_if_fail (status == CR_OK && stmt) ;
	g_return_if_fail (stmt->type == RULESET_STMT) ;

	status = cr_statement_ruleset_append_decl2
		(stmt, name, a_value) ;
	g_return_if_fail (status == CR_OK) ;

	
}

static void
parse_at_media_end_selector_cb (CRDocHandler *a_this,
				CRSelector *a_sellist)
{
	enum CRStatus status = CR_OK ;
	/*
	 *the current ruleset stmt, child of the 
	 *current at-media being parsed.
	 */
	CRStatement *stmt = NULL ;
   
	g_return_if_fail (a_this && a_sellist) ;

	status = cr_doc_handler_get_ctxt (a_this, 
					  (gpointer *)&stmt) ;
	g_return_if_fail (status == CR_OK && stmt 
			  && stmt->type == RULESET_STMT) ;
	g_return_if_fail (stmt->kind.ruleset->parent_media_rule) ;

	status = cr_doc_handler_set_ctxt 
		(a_this, 
		 stmt->kind.ruleset->parent_media_rule) ;
	g_return_if_fail (status == CR_OK) ;
}

static void
parse_at_media_end_media_cb (CRDocHandler *a_this,
			     GList *a_media_list)
{
	enum CRStatus status = CR_OK ;
	CRStatement *at_media = NULL ;
	g_return_if_fail (a_this && a_this->priv) ;

	status = cr_doc_handler_get_ctxt (a_this, 
					  (gpointer*)&at_media) ;
	g_return_if_fail (status == CR_OK && at_media) ;
	
	status = cr_doc_handler_set_result (a_this, at_media) ;
}


static void
parse_ruleset_start_selector_cb (CRDocHandler *a_this, 
				 CRSelector *a_sellist)
{
	CRStatement *ruleset = NULL ;

	g_return_if_fail (a_this 
			  && a_this->priv
			  && a_sellist) ;
	
	ruleset = cr_statement_new_ruleset (NULL, a_sellist,
					    NULL, NULL) ;
	g_return_if_fail (ruleset) ;

	cr_doc_handler_set_result (a_this, ruleset) ;	
}

static void
parse_ruleset_unrecoverable_error_cb (CRDocHandler *a_this)
{
        CRStatement *stmt = NULL ;
        enum CRStatus status = CR_OK ;

        status = cr_doc_handler_get_result (a_this, (gpointer*)&stmt) ;
        if (status != CR_OK)
        {
                cr_utils_trace_info ("Couldn't get parsing context. "
                                     "This may lead to some memory leaks.") ;
                return ;
        }
        if (stmt)
        {
                cr_statement_destroy (stmt) ;
                stmt = NULL ;
                cr_doc_handler_set_result (a_this, NULL) ;
        }
}

static void
parse_ruleset_property_cb (CRDocHandler *a_this,
			   GString *a_name, CRTerm *a_value)
{
	enum CRStatus status = CR_OK ;
	CRStatement *ruleset = NULL ;
	GString * stringue = NULL ;

	g_return_if_fail (a_this && a_this->priv && a_name) ;

	stringue = g_string_new (a_name->str) ;
	g_return_if_fail (stringue) ;

	status = cr_doc_handler_get_result (a_this, (gpointer *)&ruleset) ;
	g_return_if_fail (status == CR_OK
			  && ruleset && ruleset->type == RULESET_STMT) ;

	status = cr_statement_ruleset_append_decl2 
		(ruleset, stringue, a_value) ;
	g_return_if_fail (status == CR_OK) ;
}

static void
parse_ruleset_end_selector_cb (CRDocHandler *a_this, 
			       CRSelector *a_sellist)
{
	CRStatement *result = NULL ;
	enum CRStatus status = CR_OK ;

	g_return_if_fail (a_this && a_sellist) ;

	status = cr_doc_handler_get_result (a_this, (gpointer *)&result) ;

	g_return_if_fail (status == CR_OK 
			  && result 
			  && result->type == RULESET_STMT) ;
}

static void
cr_statement_clear (CRStatement *a_this)
{
	g_return_if_fail (a_this) ;

	switch (a_this->type)
	{
	case AT_RULE_STMT:
		break ;
	case RULESET_STMT:
		if (!a_this->kind.ruleset)
			return ;
		if (a_this->kind.ruleset->sel_list)
		{
			cr_selector_unref 
				(a_this->kind.ruleset->sel_list) ;
			a_this->kind.ruleset->sel_list = NULL ;
		}
		if (a_this->kind.ruleset->decl_list)
		{
			cr_declaration_destroy
				(a_this->kind.ruleset->decl_list) ;
			a_this->kind.ruleset->decl_list = NULL ;
		}
		g_free (a_this->kind.ruleset) ;
		a_this->kind.ruleset = NULL ;
		break ;

	case AT_IMPORT_RULE_STMT:
		if (!a_this->kind.import_rule)
			return ;
		if (a_this->kind.import_rule->url)
		{
			g_string_free 
				(a_this->kind.import_rule->url,
				 TRUE) ;
			a_this->kind.import_rule->url = NULL ;
		}
		g_free (a_this->kind.import_rule) ;
		a_this->kind.import_rule = NULL ;
		break ;

	case AT_MEDIA_RULE_STMT:
		if (!a_this->kind.media_rule)
			return;
		if (a_this->kind.media_rule->rulesets)
		{
			cr_statement_destroy 
				(a_this->kind.media_rule->rulesets) ;
			a_this->kind.media_rule->rulesets = NULL ;
		}
		if (a_this->kind.media_rule->media_list)
		{
			GList *cur = NULL ;
			
			for (cur = a_this->kind.media_rule->media_list;
			     cur ; cur = cur->next)
			{
				if (cur->data)
				{
					g_string_free ((GString*)cur->data,
						       TRUE) ;
					cur->data = NULL ;
				}
				
			}
			g_list_free 
				(a_this->kind.media_rule->media_list) ;
			a_this->kind.media_rule->media_list = NULL ;
		}
		g_free (a_this->kind.media_rule) ;
		a_this->kind.media_rule = NULL ;
		break ;

	case AT_PAGE_RULE_STMT:
		if (!a_this->kind.page_rule)
			return ;

		if (a_this->kind.page_rule->decl_list)
		{
			cr_declaration_destroy
				(a_this->kind.page_rule->decl_list) ;
			a_this->kind.page_rule->decl_list = NULL ;
		}
		if (a_this->kind.page_rule->name)
		{
			g_string_free (a_this->kind.page_rule->name,
				       TRUE) ;
			a_this->kind.page_rule->name = NULL ;
		}
		if (a_this->kind.page_rule->pseudo)
		{
			g_string_free (a_this->kind.page_rule->pseudo,
				       TRUE) ;
			a_this->kind.page_rule->pseudo = NULL ;
		}

		g_free (a_this->kind.page_rule) ;
		a_this->kind.page_rule = NULL ;
		break ;

	case AT_CHARSET_RULE_STMT:
		if (!a_this->kind.charset_rule)
			return ;

		if (a_this->kind.charset_rule->charset)
		{
			g_string_free 
				(a_this->kind.charset_rule->charset, 
				 TRUE) ;
			a_this->kind.charset_rule->charset = NULL ;
		}
		g_free (a_this->kind.charset_rule) ;
		a_this->kind.charset_rule = NULL;
		break ;

	case AT_FONT_FACE_RULE_STMT:
		if (!a_this->kind.font_face_rule)
			return ;

		if (a_this->kind.font_face_rule->decl_list)
		{
			cr_declaration_unref 
				(a_this->kind.font_face_rule->decl_list);
			a_this->kind.font_face_rule->decl_list = NULL ;
		}
		g_free (a_this->kind.font_face_rule) ;
		a_this->kind.font_face_rule = NULL ;
		break ;

	default:
		break ;
	}
}

/**
 *Dumps a ruleset statement to a file.
 *@param a_this the current instance of #CRStatement.
 *@param a_fp the destination file pointer.
 *@param a_indent the number of indentation white spaces to add.
 */
static void
cr_statement_dump_ruleset (CRStatement *a_this, FILE *a_fp, glong a_indent)
{
	guchar *str = NULL, *tmp_str = NULL ;

	g_return_if_fail (a_this && a_this->type == RULESET_STMT) ;

	if (a_this->kind.ruleset->sel_list)
	{
		if (a_indent)
			cr_utils_dump_n_chars (' ', a_fp, a_indent) ;

		cr_selector_dump (a_this->kind.ruleset->sel_list, a_fp) ;
	}

	if (a_this->kind.ruleset->decl_list)
	{
		fprintf (a_fp," {\n") ;
		cr_declaration_dump (a_this->kind.ruleset->decl_list,
				     a_fp, a_indent + DECLARATION_INDENT_NB,
				     TRUE) ;
		fprintf (a_fp,"\n") ;
		cr_utils_dump_n_chars (' ', a_fp, a_indent) ;
		fprintf (a_fp,"}") ;
	}

	if (str)
	{
		g_free (str) ;
		str = NULL ;
	}
	if (tmp_str)
	{
		g_free (tmp_str) ;
		tmp_str = NULL ;
	}
}

/**
 *Dumps a font face rule statement to a file.
 *@param a_this the current instance of font face rule statement.
 *@param a_fp the destination file pointer.
 *@param a_indent the number of white space indentation.
 */
static void
cr_statement_dump_font_face_rule (CRStatement *a_this , FILE *a_fp,
				  glong a_indent)
{
	g_return_if_fail (a_this 
			  && a_this->type == AT_FONT_FACE_RULE_STMT) ;
	
	if (a_this->kind.font_face_rule->decl_list)
	{
		cr_utils_dump_n_chars (' ', a_fp, a_indent) ;

		if (a_indent)
			cr_utils_dump_n_chars (' ', a_fp, a_indent) ;
		
		fprintf (a_fp,"@font-face {\n") ;
		cr_declaration_dump 
			(a_this->kind.font_face_rule->decl_list,
			 a_fp, a_indent + DECLARATION_INDENT_NB, TRUE) ;
		fprintf (a_fp,"\n}") ;
	}
}

/**
 *Dumps an @charset rule statement to a file.
 *@param a_this the current instance of the @charset rule statement.
 *@param a_fp the destination file pointer.
 *@param a_indent the number of indentation white spaces.
 */
static void
cr_statement_dump_charset (CRStatement *a_this, FILE *a_fp,
			   gulong a_indent)
{
	guchar *str = NULL ;

	g_return_if_fail (a_this 
			  && a_this->type == AT_CHARSET_RULE_STMT) ;
	
	if (a_this->kind.charset_rule
	    && a_this->kind.charset_rule->charset)
	{
		str = g_strndup (a_this->kind.charset_rule->charset->str,
				 a_this->kind.charset_rule->charset->len) ;

		g_return_if_fail (str) ;

		cr_utils_dump_n_chars (' ', a_fp, a_indent) ;
		fprintf (a_fp,"@charset \"%s\" ;", str) ;
		if (str)
		{
			g_free (str) ;
			str = NULL; 
		}
	}
}

/**
 *Dumps an @page rule statement on stdout.
 *@param a_this the statement to dump on stdout.
 *@param a_fp the destination file pointer.
 *@param a_indent the number of indentation white spaces.
 */
static void
cr_statement_dump_page (CRStatement *a_this, FILE *a_fp, gulong a_indent)
{
	guchar *str = NULL ;

	g_return_if_fail (a_this 
			  && a_this->type == AT_PAGE_RULE_STMT
			  && a_this->kind.page_rule) ;

	cr_utils_dump_n_chars (' ', a_fp, a_indent) ;
	fprintf (a_fp,"@page") ;

	if (a_this->kind.page_rule->name)
	{
		str = g_strndup (a_this->kind.page_rule->name->str,
				 a_this->kind.page_rule->name->len) ;
		g_return_if_fail (str) ;
		fprintf (a_fp," %s", str) ;
		if (str)
		{
			g_free (str) ;
			str = NULL ;
		}
	}
	else
	{
		fprintf (a_fp," ") ;
	}

	if (a_this->kind.page_rule->pseudo)
	{
		str = g_strndup (a_this->kind.page_rule->pseudo->str,
				 a_this->kind.page_rule->pseudo->len) ;
		g_return_if_fail (str) ;
		fprintf (a_fp,":%s", str) ;
		if (str)
		{
			g_free (str) ;
			str = NULL ;
		}
	}

	if (a_this->kind.page_rule->decl_list)
	{
		fprintf (a_fp," {\n") ;
		cr_declaration_dump 
			(a_this->kind.page_rule->decl_list,
			 a_fp, a_indent + DECLARATION_INDENT_NB, TRUE) ;
		fprintf (a_fp,"\n}\n") ;
	}
}

/**
 *Dumps an @media rule statement to a file.
 *@param a_this the statement to dump.
 *@param a_fp the destination file pointer
 *@param a_indent the number of white spaces indentation.
 */
static void
cr_statement_dump_media_rule (CRStatement *a_this, FILE *a_fp,
			      gulong a_indent)
{
	GList *cur = NULL ;

	g_return_if_fail (a_this->type == AT_MEDIA_RULE_STMT) ;
	
	if (a_this->kind.media_rule)
	{
		cr_utils_dump_n_chars (' ', a_fp, a_indent) ;
		fprintf (a_fp,"@media") ;
		for (cur = a_this->kind.media_rule->media_list ; cur ;
			cur = cur->next)
		{
			if (cur->data)
			{
				guchar *str = g_strndup 
					(((GString*)cur->data)->str,
					 ((GString*)cur->data)->len) ;
				if (str)
				{
					if (cur->prev)
					{
						fprintf (a_fp,",") ;
					}
					fprintf (a_fp," %s", str) ;
					g_free (str) ; str = NULL ;
				}
			}
		}
		fprintf (a_fp," {\n") ;
		cr_statement_dump (a_this->kind.media_rule->rulesets, 
				   a_fp, a_indent + DECLARATION_INDENT_NB) ;
		fprintf (a_fp,"\n}") ;
	}
}

/**
 *Dumps an @import rule statement to a file.
 *@param a_fp the destination file pointer.
 *@param a_indent the number of white space indentations.
 */
static void
cr_statement_dump_import_rule (CRStatement *a_this, FILE *a_fp,
			       gulong a_indent)
{
	g_return_if_fail (a_this 
			  && a_this->type == AT_IMPORT_RULE_STMT
			  && a_this->kind.import_rule) ;

	if (a_this->kind.import_rule->url)
	{
		guchar *str = NULL ;

		str = g_strndup (a_this->kind.import_rule->url->str,
				 a_this->kind.import_rule->url->len) ;
		cr_utils_dump_n_chars (' ', a_fp, a_indent) ;

		if (str)
		{
			fprintf (a_fp,"@import url(\"%s\")", str) ;
			g_free (str) ;
		}
		else /*there is no url, so no import rule, get out!*/
			return ;

		if (a_this->kind.import_rule->media_list)
		{
			GList *cur = NULL ;

			for (cur = a_this->kind.import_rule->media_list ;
			     cur; cur = cur->next)
			{
				if (cur->data)
				{
					GString *gstr = cur->data ;

					if (cur->prev)
					{
						fprintf (a_fp,", ") ;
					}

					str = g_strndup (gstr->str,
							 gstr->len) ;
					if (str)
					{
						fprintf (a_fp,str) ;
						g_free (str) ;
					}
				}
			}
			fprintf (a_fp," ;") ;
		}
	}
}


/*******************
 *public functions
 ******************/

/**
 *Tries to parse a buffer and says whether if the content of the buffer
 *is a css statement as defined by the "Core CSS Grammar" (chapter 4 of the
 *css spec) or not.
 *@param a_buf the buffer to parse.
 *@param a_encoding the character encoding of a_buf.
 *@return TRUE if the buffer parses against the core grammar, false otherwise.
 */
gboolean
cr_statement_does_buf_parses_against_core (const guchar *a_buf,
                                           enum CREncoding a_encoding)
{
        CRParser *parser = NULL ;
        enum CRStatus status = CR_OK ;
        gboolean result = FALSE ;

        parser = cr_parser_new_from_buf (a_buf, strlen (a_buf),
                                         a_encoding, FALSE) ;
        g_return_val_if_fail (parser, FALSE) ;

        status = cr_parser_set_use_core_grammar (parser, TRUE) ;
        if (status != CR_OK)
        {
                goto cleanup ;
        }
        
        status = cr_parser_parse_statement_core (parser) ;
        if (status == CR_OK)
        {
                result = TRUE ;
        }

 cleanup:
        if (parser)
        {
                cr_parser_destroy (parser) ;
        }

        return result ;
}

/**
 *Parses a buffer that contains a css statement and returns 
 *an instance of #CRStatement in case of successfull parsing.
 *TODO: at support of "@import" rules.
 *@param a_buf the buffer to parse.
 *@param a_encoding the character encoding of a_buf.
 *@return the newly built instance of #CRStatement in case
 *of successfull parsing, NULL otherwise.
 */
CRStatement *
cr_statement_parse_from_buf (const guchar *a_buf,
			     enum CREncoding a_encoding)
{
	CRStatement *result = NULL ;

	/*
	 *The strategy of this function is "brute force".
	 *It tries to parse all the types of #CRStatement it knows about.
	 *I could do this a smarter way but I don't have the time now.
	 *I think I will revisit this when time of performances and
	 *pull based incremental parsing comes.
	 */

	result = cr_statement_ruleset_parse_from_buf (a_buf, a_encoding) ;
	if (!result)
	{
		result = cr_statement_at_charset_rule_parse_from_buf 
			(a_buf, a_encoding) ;			
	}
        else
        {
                goto out ;
        }

	if (!result)
	{
		result = cr_statement_at_media_rule_parse_from_buf
			(a_buf, a_encoding) ;
	}
        else
        {
                goto out ;
        }

	if (!result)
	{
		result = cr_statement_at_charset_rule_parse_from_buf 
			(a_buf, a_encoding) ;
	}
        else
        {
                goto out ;
        }

	if (!result)
	{
		result = cr_statement_font_face_rule_parse_from_buf
			(a_buf, a_encoding) ;

	}
        else
        {
                goto out ;
        }

	if (!result)
	{
		result = cr_statement_at_page_rule_parse_from_buf
			(a_buf, a_encoding) ;
	}
        else
        {
                goto out ;
        }

        if (!result)
        {
                result = cr_statement_at_import_rule_parse_from_buf 
                        (a_buf, a_encoding) ;
        }
        else
        {
                goto out ;
        }
        
 out:
	return result ;
}

/**
 *Parses a buffer that contains a ruleset statement an instanciates
 *a #CRStatement of type RULESET_STMT.
 *@param a_buf the buffer to parse.
 *@param a_enc the character encoding of a_buf.
 *@param the newly built instance of #CRStatement in case of successfull parsing,
 *NULL otherwise.
 */
CRStatement *
cr_statement_ruleset_parse_from_buf (const guchar * a_buf,
				     enum CREncoding a_enc)
{
	enum CRStatus status = CR_OK ;
	CRStatement *result = NULL;
	CRParser *parser = NULL ;
	CRDocHandler *sac_handler = NULL ;

	g_return_val_if_fail (a_buf, NULL) ;

	parser = cr_parser_new_from_buf (a_buf, strlen (a_buf),
					 a_enc, FALSE) ;

	g_return_val_if_fail (parser, NULL) ;

	sac_handler = cr_doc_handler_new () ;
	g_return_val_if_fail (parser, NULL) ;

	sac_handler->start_selector = parse_ruleset_start_selector_cb ;
	sac_handler->end_selector = parse_ruleset_end_selector_cb ;
	sac_handler->property = parse_ruleset_property_cb ;
        sac_handler->unrecoverable_error = 
                parse_ruleset_unrecoverable_error_cb ;
	
	cr_parser_set_sac_handler (parser, sac_handler) ;
	cr_parser_try_to_skip_spaces_and_comments (parser) ;
	status = cr_parser_parse_ruleset (parser) ;
	if (status != CR_OK)
        {
                goto cleanup ;
        }

	status = cr_doc_handler_get_result (sac_handler, 
					    (gpointer*)&result) ;
	if (! ((status == CR_OK) && result) )
	{
		if (result)
		{
			cr_statement_destroy (result) ;
			result = NULL ;
		}
	}

 cleanup:
	if (parser)
	{
		cr_parser_destroy (parser) ;
		parser = NULL ;
	}
	if (sac_handler)
	{
		cr_doc_handler_unref (sac_handler) ;
		sac_handler = NULL ;
	}
	return result ;
}

/**
 *Creates a new instance of #CRStatement of type
 *#CRRulSet.
 *@param a_sel_list the list of #CRSimpleSel (selectors)
 *the rule applies to.
 *@param a_decl_list the list of instances of #CRDeclaration
 *that composes the ruleset.
 *@param a_media_types a list of instances of GString that
 *describe the media list this ruleset applies to.
 *@return the new instance of #CRStatement or NULL if something
 *went wrong.
 */
CRStatement*
cr_statement_new_ruleset (CRStyleSheet * a_sheet,
			  CRSelector *a_sel_list, 
			  CRDeclaration *a_decl_list,
			  CRStatement *a_parent_media_rule)
{
	CRStatement *result = NULL ;

	g_return_val_if_fail (a_sel_list, NULL) ;

	if (a_parent_media_rule)
	{
		g_return_val_if_fail 
			(a_parent_media_rule->type == AT_MEDIA_RULE_STMT,
			 NULL) ;
		g_return_val_if_fail (a_parent_media_rule->kind.media_rule,
				      NULL) ;
	}

	result = g_try_malloc (sizeof (CRStatement)) ;

	if (!result)
	{
		cr_utils_trace_info ("Out of memory") ;
		return NULL ;
	}

	memset (result, 0, sizeof (CRStatement)) ;
	result->type = RULESET_STMT ;
	result->kind.ruleset = g_try_malloc (sizeof (CRRuleSet)) ;

	if (!result->kind.ruleset)
	{
		cr_utils_trace_info ("Out of memory") ;
		if (result) g_free (result) ;
		return NULL ;
	}

	memset (result->kind.ruleset, 0, sizeof (CRRuleSet)) ;
	result->kind.ruleset->sel_list = a_sel_list;
	if (a_sel_list)
		cr_selector_ref (a_sel_list) ;
	result->kind.ruleset->decl_list = a_decl_list;

	if (a_parent_media_rule)
	{
		result->kind.ruleset->parent_media_rule = 
			a_parent_media_rule;
		a_parent_media_rule->kind.media_rule->rulesets = 
			cr_statement_append 
			(a_parent_media_rule->kind.media_rule->rulesets,
			 result) ;		
	}
	

	cr_statement_set_parent_sheet (result, a_sheet) ;

	return result ;
}

/**
 *Parses a buffer that contains an "@media" declaration
 *and builds an @media css statement.
 *@param a_buf the input to parse.
 *@param a_enc the encoding of the buffer.
 *@return the @media statement, or NULL if the buffer could not
 *be successfully parsed.
 */
CRStatement *
cr_statement_at_media_rule_parse_from_buf (const guchar *a_buf,
					   enum CREncoding a_enc)
{
	CRParser *parser = NULL ;
	CRStatement *result = NULL ;
	CRDocHandler *sac_handler = NULL ;
	enum CRStatus status = CR_OK ;

	parser = cr_parser_new_from_buf (a_buf, strlen (a_buf),
					 a_enc, FALSE) ;
	if (!parser)
	{
		cr_utils_trace_info ("Instanciation of the parser failed") ;
		goto cleanup ;
	}

	sac_handler = cr_doc_handler_new () ;
	if (!sac_handler)
	{
		cr_utils_trace_info ("Instanciation of the sac handler failed") ;
		goto cleanup ;
	}

	sac_handler->start_media = 
		parse_at_media_start_media_cb ;
	sac_handler->start_selector = 
		parse_at_media_start_selector_cb ;
	sac_handler->property =
		parse_at_media_property_cb ;
	sac_handler->end_selector =
		parse_at_media_end_selector_cb ;
	sac_handler->end_media = 
		parse_at_media_end_media_cb ;
        sac_handler->unrecoverable_error =
                parse_at_media_unrecoverable_error_cb ;

	status = cr_parser_set_sac_handler (parser, sac_handler) ;
	if (status != CR_OK)
		goto cleanup ;
	
	status = cr_parser_try_to_skip_spaces_and_comments (parser) ;
	if (status != CR_OK)
		goto cleanup ;

	status = cr_parser_parse_media (parser) ;
	if (status != CR_OK)
		goto cleanup ;
	
	status = cr_doc_handler_get_result (sac_handler, 
					    (gpointer*)&result) ;
	if (status != CR_OK)
		goto cleanup ;

	
 cleanup:

	if (parser)
	{
		cr_parser_destroy (parser) ;
		parser = NULL ;
	}
	if (sac_handler)
	{
		cr_doc_handler_unref (sac_handler) ;
		sac_handler = NULL ;
	}

	return result ;	
}

/**
 *Instanciates an instance of #CRStatement of type
 *AT_MEDIA_RULE_STMT (@media ruleset).
 *@param a_ruleset the ruleset statements contained
 *in the @media rule.
 *@param a_media, the media string list. A list of GString pointers.
 */
CRStatement *
cr_statement_new_at_media_rule (CRStyleSheet *a_sheet,
				CRStatement *a_rulesets,
				GList *a_media)
{
	CRStatement *result = NULL, *cur = NULL ;

	if (a_rulesets)
		g_return_val_if_fail (a_rulesets->type == RULESET_STMT,
				      NULL) ;

	result = g_try_malloc (sizeof (CRStatement)) ;

	if (!result)
	{
		cr_utils_trace_info ("Out of memory") ;
		return NULL ;
	}

	memset (result, 0, sizeof (CRStatement)) ;
	result->type = AT_MEDIA_RULE_STMT ;
	
	result->kind.media_rule = g_try_malloc (sizeof (CRAtMediaRule)) ;
	if (!result->kind.media_rule)
	{
		cr_utils_trace_info ("Out of memory") ;
		g_free (result) ;
		return NULL ;
	}
	memset (result->kind.media_rule, 0, sizeof (CRAtMediaRule)) ;
	result->kind.media_rule->rulesets = a_rulesets ;
	for (cur = a_rulesets ; cur ; cur = cur->next)
	{
		if (cur->type != RULESET_STMT || !cur->kind.ruleset)
		{
			cr_utils_trace_info ("Bad parameter a_rulesets. "
					     "It should be a list of "
					     "correct ruleset statement only !");
			goto error ;
		}
		cur->kind.ruleset->parent_media_rule = result ;
	}

	result->kind.media_rule->media_list = a_media ;	
	if (a_sheet)
	{
		cr_statement_set_parent_sheet (result, a_sheet) ;
	}

	return result ;
	
 error:
	return NULL ;
}


/**
 *Creates a new instance of #CRStatment of type
 *#CRAtImportRule.
 *@param a_url the url to connect to the get the file
 *to be imported.
 *@param a_sheet the imported parsed stylesheet.
 *@return the newly built instance of #CRStatement.
 */
CRStatement*
cr_statement_new_at_import_rule (CRStyleSheet *a_container_sheet,
				 GString *a_url,
				 GList *a_media_list,
				 CRStyleSheet *a_imported_sheet)
{
	CRStatement *result = NULL ;

	result = g_try_malloc (sizeof (CRStatement)) ;

	if (!result)
	{
		cr_utils_trace_info ("Out of memory") ;
		return NULL ;
	}

	memset (result, 0, sizeof (CRStatement)) ;
	result->type = AT_IMPORT_RULE_STMT ;

	result->kind.import_rule = 
		g_try_malloc (sizeof (CRAtImportRule)) ;

	if (!result->kind.import_rule)
	{
		cr_utils_trace_info ("Out of memory") ;
		g_free (result) ;
		return NULL ;
	}

	memset (result->kind.import_rule, 0, sizeof (CRAtImportRule)) ;
	result->kind.import_rule->url = a_url;
	result->kind.import_rule->media_list = a_media_list ;
	result->kind.import_rule->sheet = a_imported_sheet;
        if (a_container_sheet)
                cr_statement_set_parent_sheet (result, a_container_sheet) ;

	return result ;
}

/**
 *Parses a buffer that contains an "@import" rule and
 *instanciate a #CRStatement of type AT_IMPORT_RULE_STMT
 *@param a_buf the buffer to parse.
 *@param a_encoding the encoding of a_buf.
 *@return the newly built instance of #CRStatement in case of 
 *a successfull parsing, NULL otherwise.
 */
CRStatement *
cr_statement_at_import_rule_parse_from_buf (const guchar * a_buf,
                                            enum CREncoding a_encoding)
{
        enum CRStatus status = CR_OK ;
	CRParser *parser = NULL ;
        CRStatement *result = NULL ;
        GList *media_list = NULL ;
        GString *import_string = NULL ;

        parser = cr_parser_new_from_buf (a_buf, strlen (a_buf),
                                         a_encoding, FALSE) ;
        if (!parser)
        {
                cr_utils_trace_info ("Instanciation of parser failed.") ;
                goto cleanup ;
        }

        status = cr_parser_try_to_skip_spaces_and_comments (parser) ;
        if (status != CR_OK)
                goto cleanup ;

        status = cr_parser_parse_import (parser, &media_list,
                                         &import_string) ;
        if (status != CR_OK || !import_string)
                goto cleanup ;

        result = cr_statement_new_at_import_rule (NULL, import_string,
                                                  media_list, NULL) ;
        if (result)
        {
                import_string = NULL ;
                media_list = NULL ;
        }

 cleanup:
        if (parser)
        {
                cr_parser_destroy (parser) ;
                parser = NULL ;
        }
        if (media_list)
        {
                GList *cur = NULL ;
                for (cur = media_list; media_list; 
                     media_list = g_list_next (media_list))
                {
                        if (media_list->data)
                        {
                                g_string_free (media_list->data, TRUE);
                                media_list->data = NULL ;
                        }
                }
                g_list_free (media_list) ;
                media_list = NULL;
        }
        if (import_string)
        {
                g_string_free (import_string, TRUE) ;
                import_string = NULL;
        }

        return result ;
}

/**
 *Creates a new instance of #CRStatement of type
 *#CRAtPageRule.
 *@param a_decl_list a list of instances of #CRDeclarations
 *which is actually the list of declarations that applies to
 *this page rule.
 *@param a_selector the page rule selector.
 *@return the newly built instance of #CRStatement or NULL
 *in case of error.
 */
CRStatement *
cr_statement_new_at_page_rule (CRStyleSheet *a_sheet,
	                       CRDeclaration *a_decl_list,
			       GString *a_name,
			       GString *a_pseudo)
{
	CRStatement *result = NULL ;


	result = g_try_malloc (sizeof (CRStatement)) ;

	if (!result)
	{
		cr_utils_trace_info ("Out of memory") ;
		return NULL ;
	}

	memset (result, 0, sizeof (CRStatement)) ;
	result->type = AT_PAGE_RULE_STMT ;

	result->kind.page_rule = g_try_malloc (sizeof (CRAtPageRule)) ;

	if (!result->kind.page_rule)
	{
		cr_utils_trace_info ("Out of memory") ;
		g_free (result) ;
		return NULL ;
	}

	memset (result->kind.page_rule, 0, sizeof (CRAtPageRule)) ;
	if (a_decl_list)
	{
		result->kind.page_rule->decl_list = a_decl_list;
		cr_declaration_ref (a_decl_list) ;
	}
	result->kind.page_rule->name = a_name ;
	result->kind.page_rule->name = a_pseudo ;
	if (a_sheet)
		cr_statement_set_parent_sheet (result, a_sheet) ;

	return result ;
}

/**
 *Parses a buffer that contains an "@page" production and,
 *if the parsing succeeds, builds the page statement.
 *@param a_buf the character buffer to parse.
 *@param a_encoding the character encoding of a_buf.
 *@return the newly built at page statement in case of successfull parsing,
 *NULL otherwise.
 */
CRStatement *
cr_statement_at_page_rule_parse_from_buf (const guchar *a_buf,
					  enum CREncoding a_encoding) 
{
	enum CRStatus status = CR_OK ;
	CRParser *parser = NULL ;
	CRDocHandler *sac_handler = NULL ;
	CRStatement *result = NULL ;

	g_return_val_if_fail (a_buf, NULL) ;
	
	parser = cr_parser_new_from_buf (a_buf, strlen (a_buf),
					 a_encoding, FALSE) ;
	if (!parser)
	{
		cr_utils_trace_info ("Instanciation of the parser failed.") ;
		goto cleanup ;
	}

	sac_handler = cr_doc_handler_new () ;
	if (!sac_handler)
	{
		cr_utils_trace_info 
			("Instanciation of the sac handler failed.") ;
		goto cleanup ;
	}

	sac_handler->start_page = 
		parse_page_start_page_cb ;
	sac_handler->property =
		parse_page_property_cb ;
	sac_handler->end_page = 
		parse_page_end_page_cb ;
        sac_handler->unrecoverable_error =
                parse_page_unrecoverable_error_cb ;

	status = cr_parser_set_sac_handler (parser, sac_handler) ;
	if (status != CR_OK)
		goto cleanup ;
	
	/*Now, invoke the parser to parse the "@page production"*/
	cr_parser_try_to_skip_spaces_and_comments (parser) ;
	if (status != CR_OK)
		goto cleanup ;
	status = cr_parser_parse_page (parser) ;
	if (status != CR_OK)
		goto cleanup ;
	
	status = cr_doc_handler_get_result (sac_handler,
					    (gpointer*)&result) ;

 cleanup:

	if (parser)
	{
		cr_parser_destroy (parser) ;
		parser = NULL ;
	}
	if (sac_handler)
	{
		cr_doc_handler_unref (sac_handler) ;
		sac_handler = NULL ;
	}
	return result  ;
	
}

/**
 *Creates a new instance of #CRStatement of type
 *#CRAtCharsetRule.
 *@param a_charset the string representing the charset.
 *Note that the newly built instance of #CRStatement becomes
 *the owner of a_charset. The caller must not free a_charset !!!.
 *@return the newly built instance of #CRStatement or NULL
 *if an error arises.
 */
CRStatement *
cr_statement_new_at_charset_rule (CRStyleSheet *a_sheet,
				  GString *a_charset)
{
	CRStatement * result = NULL ;

	g_return_val_if_fail (a_charset, NULL) ;

	result = g_try_malloc (sizeof (CRStatement)) ;

	if (!result)
	{
		cr_utils_trace_info ("Out of memory") ;
		return NULL ;
	}

	memset (result, 0, sizeof (CRStatement)) ;
	result->type = AT_CHARSET_RULE_STMT ;

	result->kind.charset_rule = g_try_malloc 
		(sizeof (CRAtCharsetRule)) ;

	if (!result->kind.charset_rule)
	{
		cr_utils_trace_info ("Out of memory") ;
		g_free (result) ;
		return NULL ;
	}
	memset (result->kind.charset_rule, 0, sizeof (CRAtCharsetRule)) ;
	result->kind.charset_rule->charset = a_charset ;
	cr_statement_set_parent_sheet (result, a_sheet) ;

	return result ;
}

/**
 *Parses a buffer that contains an '@charset' rule and
 *creates an instance of #CRStatement of type AT_CHARSET_RULE_STMT.
 *@param a_buf the buffer to parse.
 *@param the character encoding of the buffer.
 *@return the newly built instance of #CRStatement.
 */
CRStatement *
cr_statement_at_charset_rule_parse_from_buf (const guchar *a_buf,
					     enum CREncoding a_encoding) 
{
	enum CRStatus status = CR_OK ;
	CRParser *parser = NULL ;
	CRStatement *result = NULL ;
	GString *charset = NULL ;

	g_return_val_if_fail (a_buf, NULL) ;	
	
	parser = cr_parser_new_from_buf (a_buf, strlen (a_buf),
					 a_encoding, FALSE) ;
	if (!parser)
	{
		cr_utils_trace_info ("Instanciation of the parser failed.") ;
		goto cleanup ;
	}
	
	/*Now, invoke the parser to parse the "@charset production"*/
	cr_parser_try_to_skip_spaces_and_comments (parser) ;
	if (status != CR_OK)
		goto cleanup ;
	status = cr_parser_parse_charset (parser, &charset) ;
	if (status != CR_OK || !charset)
		goto cleanup ;

	result = cr_statement_new_at_charset_rule (NULL, charset) ;
	if (result)
		charset = NULL ;

 cleanup:

	if (parser)
	{
		cr_parser_destroy (parser) ;
		parser = NULL ;
	}
	if (charset)
	{
		g_string_free (charset, TRUE) ;
	}

	return result  ;
}

/**
 *Creates an instance of #CRStatement of type #CRAtFontFaceRule.
 *@param a_font_decls a list of instances of #CRDeclaration. Each declaration
 *is actually a font declaration.
 *@return the newly built instance of #CRStatement.
 */
CRStatement *
cr_statement_new_at_font_face_rule (CRStyleSheet *a_sheet,
				    CRDeclaration *a_font_decls)
{
	CRStatement *result = NULL ;
	
	result = g_try_malloc (sizeof (CRStatement)) ;

	if (!result)
	{
		cr_utils_trace_info ("Out of memory") ;
		return NULL ;
	}
	memset (result, 0, sizeof (CRStatement)) ;
	result->type = AT_FONT_FACE_RULE_STMT ;

	result->kind.font_face_rule = g_try_malloc 
		(sizeof (CRAtFontFaceRule)) ;

	if (!result->kind.font_face_rule)
	{
		cr_utils_trace_info ("Out of memory") ;
		g_free (result) ;
		return NULL ;
	}
	memset (result->kind.font_face_rule, 0, 
		sizeof (CRAtFontFaceRule));
	
	result->kind.font_face_rule->decl_list = a_font_decls ;
	if (a_sheet)
		cr_statement_set_parent_sheet (result, a_sheet) ;

	return result ;
}

/**
 *Parses a buffer that contains an "@font-face" rule and builds
 *an instance of #CRStatement of type AT_FONT_FACE_RULE_STMT out of it.
 *@param a_buf the buffer to parse.
 *@param a_encoding the character encoding of a_buf.
 *@return the newly built instance of #CRStatement in case of successufull
 *parsing, NULL otherwise.
 */
CRStatement *
cr_statement_font_face_rule_parse_from_buf (const guchar *a_buf,
					    enum CREncoding a_encoding)
{
	CRStatement *result = NULL ;
	CRParser *parser = NULL ;
	CRDocHandler *sac_handler = NULL ;
	enum CRStatus status = CR_OK ;

	parser = cr_parser_new_from_buf (a_buf, strlen (a_buf),
					 a_encoding, FALSE) ;
	if (!parser)
		goto cleanup ;

	sac_handler = cr_doc_handler_new () ;
	if (!sac_handler)
		goto cleanup ;

	/*
	 *set sac callbacks here
	 */
	sac_handler->start_font_face = parse_font_face_start_font_face_cb ;
	sac_handler->property = parse_font_face_property_cb ;
	sac_handler->end_font_face = parse_font_face_end_font_face_cb ;	
        sac_handler->unrecoverable_error =
                parse_font_face_unrecoverable_error_cb ;

	status = cr_parser_set_sac_handler (parser, sac_handler) ;
	if (status != CR_OK)
		goto cleanup ;

	/*
	 *cleanup spaces of comment that may be there before the real
	 *"@font-face" thing.
	 */
	status = cr_parser_try_to_skip_spaces_and_comments (parser) ;
	if (status != CR_OK)
		goto cleanup ;

	status = cr_parser_parse_font_face (parser) ;
	if (status != CR_OK)
		goto cleanup ;

	status = cr_doc_handler_get_result (sac_handler, (gpointer*)&result) ;
	if (status != CR_OK || !result)
		goto cleanup ;

 cleanup:
	if (parser)
	{
		cr_parser_destroy (parser) ;
		parser = NULL ;
	}
	if (sac_handler)
	{
		cr_doc_handler_unref (sac_handler) ;
		sac_handler = NULL ;
	}
	return result ;
}

/**
 *Sets the container stylesheet.
 *@param a_this the current instance of #CRStatement.
 *@param a_sheet the sheet that contains the current statement.
 *@return CR_OK upon successfull completion, an errror code otherwise.
 */
enum CRStatus
cr_statement_set_parent_sheet (CRStatement *a_this, 
			       CRStyleSheet *a_sheet)
{
	g_return_val_if_fail (a_this, CR_BAD_PARAM_ERROR) ;
	a_this->parent_sheet = a_sheet ;
	return CR_OK ;
}

/**
 *Gets the sheets that contains the current statement.
 *@param a_this the current #CRStatement.
 *@param a_sheet out parameter. A pointer to the sheets that
 *@return CR_OK upon successfull completion, an error code otherwise.
 */
enum CRStatus
cr_statement_get_parent_sheet (CRStatement *a_this, 
			       CRStyleSheet **a_sheet)
{
	g_return_val_if_fail (a_this && a_sheet, 
			      CR_BAD_PARAM_ERROR) ;
	*a_sheet  = a_this->parent_sheet ;
	return CR_OK ;
}

/**
 *Appends a new statement to the statement list.
 *@param a_this the current instance of the statement list.
 *@param a_this a_new the new instance of #CRStatement to append.
 *@return the new list statement list, or NULL in cas of failure.
 */
CRStatement *
cr_statement_append (CRStatement *a_this,
		     CRStatement *a_new)
{
	CRStatement * cur = NULL ;

	g_return_val_if_fail (a_new, NULL) ;
	
	if (!a_this)
	{
		return a_new ;
	}

	/*walk forward in the current list to find the tail list element*/
	for (cur = a_this ; cur && cur->next ; cur = cur->next) ;

	cur->next = a_new ;
	a_new->prev = cur ;

	return a_this ;
}

/**
 *Prepends the an instance of #CRStatement to
 *the current statement list.
 *@param a_this the current instance of #CRStatement.
 *@param a_new the new statement to prepend.
 *@return the new list with the new statement prepended,
 *or NULL in case of an error.
 */
CRStatement*
cr_statement_prepend (CRStatement *a_this,
		      CRStatement *a_new)
{
	CRStatement *cur = NULL ;

	g_return_val_if_fail (a_new, NULL) ;

	if (!a_this)
		return a_new ;

	a_new->next = a_this ;
	a_this->prev = a_new ;

	/*walk backward in the prepended list to find the head list element*/
	for (cur = a_new ; cur && cur->prev ; cur = cur->prev) ;
	
	return cur ;
}

/**
 *Unlinks a statement from the statements list.
 *@param a_this the current statements list.
 *@param a_to_unlink the statement to unlink from the list.
 *@return the new list where a_to_unlink has been unlinked
 *from, or NULL in case of error.
 */
CRStatement *
cr_statement_unlink (CRStatement *a_stmt)
{
	CRStatement *result = a_stmt ;

	g_return_val_if_fail (result, NULL) ;

        /**
         *Some sanity checks first
         */
        if (a_stmt->next)
        {
                g_return_val_if_fail (a_stmt->next->prev == a_stmt,
                                      NULL) ;
        }
        if (a_stmt->prev)
        {
                g_return_val_if_fail (a_stmt->prev->next == a_stmt,
                                      NULL) ;
        }
        
        /**
         *Now, the real unlinking job.
         */
        if (a_stmt->next)
        {
                a_stmt->next->prev = a_stmt->prev ;
        }
        if (a_stmt->prev)
        {
                a_stmt->prev->next = a_stmt->next ;
        }
        
        if (a_stmt->parent_sheet 
            && a_stmt->parent_sheet->statements == a_stmt)
        {
                a_stmt->parent_sheet->statements = 
                        a_stmt->parent_sheet->statements->next ;
        }

        a_stmt->next = NULL ;
        a_stmt->prev = NULL ;
        a_stmt->parent_sheet = NULL ;

	return result ;
}


/**
 *Sets a selector list to a ruleset statement.
 *@param a_this the current ruleset statement.
 *@param a_sel_list the selector list to set. Note
 *that this function increments the ref count of a_sel_list.
 *The sel list will be destroyed at the destruction of the
 *current instance of #CRStatement.
 *@return CR_OK upon successfull completion, an error code otherwise.
 */
enum CRStatus
cr_statement_ruleset_set_sel_list (CRStatement *a_this,
				   CRSelector *a_sel_list)
{
	g_return_val_if_fail (a_this && a_this->type == RULESET_STMT,
			      CR_BAD_PARAM_ERROR) ;
	
	if (a_this->kind.ruleset->sel_list)
		cr_selector_unref (a_this->kind.ruleset->sel_list) ;
	
	a_this->kind.ruleset->sel_list = a_sel_list ;

	if (a_sel_list)
		cr_selector_ref (a_sel_list) ;

	return CR_OK ;	
}

/**
 *Gets a pointer to the list of declaration contained
 *in the ruleset statement.
 *@param a_this the current instance of #CRStatement.
 *@a_decl_list out parameter. A pointer to the the returned
 *list of declaration. Must not be NULL.
 *@return CR_OK upon successfull completion, an error code if something
 *bad happened.
 */
enum CRStatus
cr_statement_ruleset_get_declarations (CRStatement *a_this,
                                       CRDeclaration **a_decl_list)
{
        g_return_val_if_fail (a_this 
                              && a_this->type == RULESET_STMT
                              && a_this->kind.ruleset
                              && a_decl_list,
			      CR_BAD_PARAM_ERROR) ;

        *a_decl_list = a_this->kind.ruleset->decl_list ;

        return CR_OK ;
}

/**
 *Gets a pointer to the selector list contained in
 *the current ruleset statement.
 *@param a_this the current ruleset statement.
 *@param a_list out parameter. The returned selector list,
 *if and only if the function returned CR_OK.
 *@return CR_OK upon successfull completion, an error code otherwise.
 */
enum CRStatus
cr_statement_ruleset_get_sel_list (CRStatement *a_this,
				   CRSelector **a_list)
{
	g_return_val_if_fail (a_this && a_this->type == RULESET_STMT
			      && a_this->kind.ruleset, CR_BAD_PARAM_ERROR) ;
	
	*a_list = a_this->kind.ruleset->sel_list ;

	return CR_OK ;
}


/**
 *Sets a declaration list to the current ruleset statement.
 *@param a_this the current ruleset statement.
 *@param a_list the declaration list to be added to the current
 *ruleset statement.
 *@return CR_OK upon successfull completion, an error code otherwise.
 */
enum CRStatus
cr_statement_ruleset_set_decl_list (CRStatement *a_this,
				    CRDeclaration *a_list)
{
	g_return_val_if_fail (a_this && a_this->type == RULESET_STMT
			      && a_this->kind.ruleset, CR_BAD_PARAM_ERROR);

	if (a_this->kind.ruleset->decl_list == a_list)
		return CR_OK ;

	if (a_this->kind.ruleset->sel_list)
	{
		cr_declaration_destroy (a_this->kind.ruleset->decl_list) ;
	}

	a_this->kind.ruleset->sel_list = NULL;

	return CR_OK ;
}


/**
 *Appends a declaration to the current ruleset statement.
 *@param a_this the current statement.
 *@param a_prop the property of the declaration.
 *@param a_value the value of the declaration.
 *@return CR_OK uppon successfull completion, an error code
 *otherwise.
 */
enum CRStatus
cr_statement_ruleset_append_decl2 (CRStatement *a_this,
				   GString *a_prop, CRTerm *a_value)
{
	CRDeclaration * new_decls = NULL ;

	g_return_val_if_fail (a_this && a_this->type == RULESET_STMT
			      && a_this->kind.ruleset, CR_BAD_PARAM_ERROR) ;

	new_decls = cr_declaration_append2 
		(a_this->kind.ruleset->decl_list, a_prop, a_value) ;
	g_return_val_if_fail (new_decls, CR_ERROR) ;
	a_this->kind.ruleset->decl_list = new_decls ;

	return CR_OK ;
}

/**
 *Appends a declaration to the current statement.
 *@param a_this the current statement.
 *@param a_declaration the declaration to append.
 *@return CR_OK upon sucessfull completion, an error code
 *otherwise.
 */
enum CRStatus
cr_statement_ruleset_append_decl (CRStatement *a_this,
				  CRDeclaration *a_decl)
{
	CRDeclaration * new_decls = NULL ;

	g_return_val_if_fail (a_this && a_this->type == RULESET_STMT
			      && a_this->kind.ruleset, CR_BAD_PARAM_ERROR) ;

	new_decls = cr_declaration_append
		(a_this->kind.ruleset->decl_list, a_decl) ;
	g_return_val_if_fail (new_decls, CR_ERROR) ;
	a_this->kind.ruleset->decl_list = new_decls ;

	return CR_OK ;
}


/**
 *Sets a stylesheet to the current @import rule.
 *@param a_this the current @import rule.
 *@param a_sheet the stylesheet. The stylesheet is owned
 *by the current instance of #CRStatement, that is, the 
 *stylesheet will be destroyed when the current instance
 *of #CRStatement will be destroyed.
 *@return CR_OK upon successfull completion, an error code otherwise.
 */
enum CRStatus
cr_statement_at_import_rule_set_imported_sheet (CRStatement *a_this,
						CRStyleSheet *a_sheet)
{
	g_return_val_if_fail (a_this 
			      && a_this->type == AT_IMPORT_RULE_STMT
			      && a_this->kind.import_rule,
			      CR_BAD_PARAM_ERROR) ;

	a_this->kind.import_rule->sheet = a_sheet ;

	return CR_OK ;
}


/**
 *Gets the stylesheet contained by the @import rule statement.
 *@param a_this the current @import rule statement.
 *@param a_sheet out parameter. The returned stylesheet if and
 *only if the function returns CR_OK.
 *@return CR_OK upon sucessfull completion, an error code otherwise.
 */
enum CRStatus
cr_statement_at_import_rule_get_imported_sheet (CRStatement *a_this,
						CRStyleSheet **a_sheet)
{
	g_return_val_if_fail (a_this 
			      && a_this->type == AT_IMPORT_RULE_STMT
			      && a_this->kind.import_rule,
			      CR_BAD_PARAM_ERROR) ;
	*a_sheet = a_this->kind.import_rule->sheet ;
	return CR_OK ;

}

/**
 *Sets an url to the current @import rule statement.
 *@param a_this the current @import rule statement.
 *@param a_url the url to set.
 *@return CR_OK upon successfull completion, an error code otherwise.
 */
enum CRStatus
cr_statement_at_import_rule_set_url (CRStatement *a_this,
				     GString *a_url)
{
	g_return_val_if_fail (a_this 
			      && a_this->type == AT_IMPORT_RULE_STMT
			      && a_this->kind.import_rule,
			      CR_BAD_PARAM_ERROR) ;

	if (a_this->kind.import_rule->url)
	{
		g_string_free (a_this->kind.import_rule->url, TRUE) ;
	}
	
	a_this->kind.import_rule->url = a_url ;

	return CR_OK ;
}


/**
 *Gets the url of the @import rule statement.
 *@param the current @import rule statement.
 *@param a_url out parameter. The returned url if
 *and only if the function returned CR_OK.
 */
enum CRStatus
cr_statement_at_import_rule_get_url (CRStatement *a_this,
				     GString **a_url)
{
	g_return_val_if_fail (a_this 
			      && a_this->type == AT_IMPORT_RULE_STMT
			      && a_this->kind.import_rule,
			      CR_BAD_PARAM_ERROR) ;

	*a_url = a_this->kind.import_rule->url ;

	return CR_OK ;
}

/**
 *Sets a declaration list to the current @page rule statement.
 *@param a_this the current @page rule statement.
 *@param a_decl_list the declaration list to add. Will be freed
 *by the current instance of #CRStatement when it is destroyed.
 *@return CR_OK upon successfull completion, an error code otherwise.
 */
enum CRStatus
cr_statement_at_page_rule_set_declarations (CRStatement *a_this,
					    CRDeclaration *a_decl_list)
{
	g_return_val_if_fail (a_this
			      && a_this->type == AT_PAGE_RULE_STMT
			      && a_this->kind.page_rule,
			      CR_BAD_PARAM_ERROR) ;

	if (a_this->kind.page_rule->decl_list)
	{
		cr_declaration_unref (a_this->kind.page_rule->decl_list);
	}

	a_this->kind.page_rule->decl_list = a_decl_list ;

	if (a_decl_list)
	{
		cr_declaration_ref (a_decl_list) ;
	}

	return CR_OK ;
}


/**
 *Gets the declaration list associated to the current @page rule
 *statement.
 *@param a_this the current  @page rule statement.
 *@param a_decl_list out parameter. The returned declaration list.
 *@return CR_OK upon successfull completion, an error code otherwise.
 */
enum CRStatus
cr_statement_at_page_rule_get_declarations (CRStatement *a_this,
					    CRDeclaration **a_decl_list)
{
	g_return_val_if_fail (a_this
			      && a_this->type == AT_PAGE_RULE_STMT
			      && a_this->kind.page_rule,
			      CR_BAD_PARAM_ERROR) ;

	*a_decl_list = a_this->kind.page_rule->decl_list ;

	return CR_OK ;
}


/**
 *Sets the charset of the current @charset rule statement.
 *@param a_this the current @charset rule statement.
 *@param a_charset the charset to set.
 *@return CR_OK upon successfull completion, an error code otherwise.
 */
enum CRStatus
cr_statement_at_charset_rule_set_charset (CRStatement *a_this,
					  GString *a_charset)
{
	g_return_val_if_fail (a_this
			      && a_this->type == AT_CHARSET_RULE_STMT
			      && a_this->kind.charset_rule,
			      CR_BAD_PARAM_ERROR) ;

	if (a_this->kind.charset_rule->charset)
	{
		g_string_free (a_this->kind.charset_rule->charset,
			       TRUE) ;
	}

	a_this->kind.charset_rule->charset = a_charset ;
	return CR_OK ;
}


/**
 *Gets the charset string associated to the current
 *@charset rule statement.
 *@param a_this the current @charset rule statement.
 *@param a_charset out parameter. The returned charset string if
 *and only if the function returned CR_OK.
 */
enum CRStatus
cr_statement_at_charset_rule_get_charset (CRStatement *a_this,
					  GString **a_charset)
{
	g_return_val_if_fail (a_this
			      && a_this->type == AT_CHARSET_RULE_STMT
			      && a_this->kind.charset_rule,
			      CR_BAD_PARAM_ERROR) ;

	*a_charset = a_this->kind.charset_rule->charset ;

	return CR_OK ;
}


/**
 *Sets a declaration list to the current @font-face rule statement.
 *@param a_this the current @font-face rule statement.
 *@param a_decls the declarations list to set.
 *@return CR_OK upon successfull completion, an error code otherwise.
 */
enum CRStatus
cr_statement_at_font_face_rule_set_decls (CRStatement *a_this,
					  CRDeclaration *a_decls)
{
	g_return_val_if_fail (a_this 
			      && a_this->type == AT_FONT_FACE_RULE_STMT
			      && a_this->kind.font_face_rule,
			      CR_BAD_PARAM_ERROR) ;

	if (a_this->kind.font_face_rule->decl_list)
	{
		cr_declaration_unref 
			(a_this->kind.font_face_rule->decl_list) ;
	}

	a_this->kind.font_face_rule->decl_list = a_decls;
	cr_declaration_ref (a_decls) ;
	
	return CR_OK ;
}


/**
 *Gets the declaration list associated to the current instance
 *of @font-face rule statement.
 *@param a_this the current @font-face rule statement.
 *@param a_decls out parameter. The returned declaration list if
 *and only if this function returns CR_OK.
 *@return CR_OK upon successfull completion, an error code otherwise.
 */
enum CRStatus
cr_statement_at_font_face_rule_get_decls (CRStatement *a_this,
					  CRDeclaration **a_decls)
{
	g_return_val_if_fail (a_this 
			      && a_this->type == AT_FONT_FACE_RULE_STMT
			      && a_this->kind.font_face_rule,
			      CR_BAD_PARAM_ERROR) ;

	*a_decls = a_this->kind.font_face_rule->decl_list ;
	
	return CR_OK ;
}


/**
 *Adds a declaration to the current @font-face rule
 *statement.
 *@param a_this the current @font-face rule statement.
 *@param a_prop the property of the declaration.
 *@param a_value the value of the declaration.
 *@return CR_OK upon successfull completion, an error code otherwise.
 */
enum CRStatus
cr_statement_at_font_face_rule_add_decl (CRStatement *a_this,
					 GString *a_prop,
					 CRTerm *a_value)
{
	CRDeclaration *decls = NULL ;
	
	g_return_val_if_fail (a_this 
			      && a_this->type == AT_FONT_FACE_RULE_STMT
			      && a_this->kind.font_face_rule,
			      CR_BAD_PARAM_ERROR) ;

	decls = cr_declaration_append2 
		(a_this->kind.font_face_rule->decl_list,
		 a_prop, a_value) ;

	g_return_val_if_fail (decls, CR_ERROR) ;
		
	if (a_this->kind.font_face_rule->decl_list == NULL)
		cr_declaration_ref (decls) ;

	a_this->kind.font_face_rule->decl_list = decls ;

	return CR_OK ;
}

/**
 *Dumps the css2 statement to a file.
 *@param a_this the current css2 statement.
 *@param a_fp the destination file pointer.
 *@param a_indent the number of white space indentation characters.
 */
void
cr_statement_dump (CRStatement *a_this, FILE *a_fp, gulong a_indent)
{

        if (!a_this)
                return ;

	if (a_this->prev)
	{
		fprintf (a_fp,"\n\n") ;
	}

	switch (a_this->type)
	{
	case RULESET_STMT:
		cr_statement_dump_ruleset (a_this, a_fp, a_indent) ;
		break ;

	case AT_FONT_FACE_RULE_STMT:
		cr_statement_dump_font_face_rule 
			(a_this, a_fp, a_indent);
		break ;

	case AT_CHARSET_RULE_STMT:
		cr_statement_dump_charset (a_this, a_fp, a_indent) ;
		break ;

	case AT_PAGE_RULE_STMT:
		cr_statement_dump_page (a_this, a_fp, a_indent) ;
		break ;

	case AT_MEDIA_RULE_STMT:
		cr_statement_dump_media_rule (a_this, a_fp, a_indent) ;
		break ;

	case AT_IMPORT_RULE_STMT:
		cr_statement_dump_import_rule (a_this, a_fp, a_indent) ;
		break ;

	default :
		fprintf (a_fp, "Statement unrecognized at %s:%d", 
			 __FILE__, __LINE__) ;
		break ;
	}
}

/**
 *Destructor of #CRStatement.
 */
void
cr_statement_destroy (CRStatement *a_this)
{
	CRStatement *cur = NULL ;

	g_return_if_fail (a_this) ;
	
	/*go get the tail of the list*/
	for (cur = a_this ; cur && cur->next; cur = cur->next)
	{
		cr_statement_clear (cur) ;
	}
	
	if (cur)
		cr_statement_clear (cur) ;

	if (cur->prev == NULL)
	{
		g_free (a_this);
		return ;
	}

	/*walk backward and free next element*/
	for (cur = cur->prev ; cur && cur->prev; cur = cur->prev)
	{
		if (cur->next)
		{
			g_free (cur->next) ;
			cur->next = NULL ;
		}		
	}

	if (!cur)
		return ;

	/*free the one remaining list*/
	if (cur->next)
	{
		g_free (cur->next) ;		
		cur->next = NULL ;
	}

	g_free (cur) ;
	cur = NULL ;
}
