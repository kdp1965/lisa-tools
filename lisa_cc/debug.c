// Copyright 2012 Rui Ueyama. Released under the MIT license.

#include "lisacc.h"
#include "string.h"

static char *decorate_int(char *name, Type *ty) {
    char *u = (ty->usig) ? "u" : "";
    if (ty->bitsize > 0)
        return format("%s%s:%d:%d", u, name, ty->bitoff, ty->bitoff + ty->bitsize);
    return format("%s%s", u, name);
}

static char *do_ty2s(Dict *dict, Type *ty) {
    if (!ty)
        return "(nil)";
    switch (ty->kind) {
    case KIND_VOID: return "void";
    case KIND_BOOL: return "_Bool";
    case KIND_CHAR: return decorate_int("char", ty);
    case KIND_SHORT: return decorate_int("short", ty);
    case KIND_INT:  return decorate_int("int", ty);
    case KIND_LONG: return decorate_int("long", ty);
    case KIND_LLONG: return decorate_int("llong", ty);
    case KIND_FLOAT: return "float";
    case KIND_DOUBLE: return "double";
    case KIND_LDOUBLE: return "long double";
    case KIND_PTR:
        return format("*%s", do_ty2s(dict, ty->ptr));
    case KIND_ARRAY:
        return format("[%d]%s", ty->len, do_ty2s(dict, ty->ptr));
    case KIND_STRUCT: {
        char *kind = ty->is_struct ? "struct" : "union";
        if (dict_get(dict, format("%p", ty)))
            return format("(%s)", kind);
        dict_put(dict, format("%p", ty), (void *)1);
        if (ty->fields) {
            Buffer *b = make_buffer();
            buf_printf(b, "(%s", kind);
            Vector *keys = dict_keys(ty->fields);
            for (int i = 0; i < vec_len(keys); i++) {
                char *key = vec_get(keys, i);
                Type *fieldtype = dict_get(ty->fields, key);
                buf_printf(b, " (%s)", do_ty2s(dict, fieldtype));
            }
            buf_printf(b, ")");
            return buf_body(b);
        }
    }
    case KIND_FUNC: {
        Buffer *b = make_buffer();
        buf_printf(b, "(");
        if (ty->params) {
            for (int i = 0; i < vec_len(ty->params); i++) {
                if (i > 0)
                    buf_printf(b, ",");
                Type *t = vec_get(ty->params, i);
                buf_printf(b, "%s", do_ty2s(dict, t));
            }
        }
        buf_printf(b, ")=>%s", do_ty2s(dict, ty->rettype));
        return buf_body(b);
    }
    default:
        return format("(Unknown ty: %d)", ty->kind);
    }
}

char *ty2class(Type *ty) {
    if (!ty)
        return "(nil)";
    if (ty->isstatic)
      return "static ";
    if (ty->isregister)
      return "register ";
    if (ty->isaccumulator)
      return "accumulator ";
    if (ty->issfr)
      return "sfr ";
    return "";
}

char *ty2s(Type *ty) {
    return do_ty2s(make_dict(), ty);
}

static void uop_to_string(Buffer *b, char *op, Node *node, int indent) {
    char sIndent[16];
    buf_printf(b, "(%s %s", op, node2s(node->operand, indent == -1 ? -1 : indent+2));
    if (indent != -1)
    {
        sprintf(sIndent, "\n%%%ds", indent);
        buf_printf(b, sIndent, " ");
    }
    buf_printf(b, ")");
}

static void binop_to_string(Buffer *b, char *op, Node *node, int indent) {
    char sIndent[16];

    buf_printf(b, "(%s %s %s", op, node2s(node->left, indent == -1 ? -1 : indent+2), node2s(node->right, indent == -1 ? -1 : indent+2));
    if (indent != -1)
    {
        sprintf(sIndent, "\n%%%ds", indent);
        buf_printf(b, sIndent, " ");
    }
    buf_printf(b, ")");
}

static void a2s_declinit(Buffer *b, Vector *initlist, int indent) {
    for (int i = 0; i < vec_len(initlist); i++) {
        if (i > 0)
            buf_printf(b, " ");
        Node *init = vec_get(initlist, i);
        buf_printf(b, "%s", node2s(init, indent == -1 ? -1 : indent+2));
    }
}

static void do_ast(Buffer *b, const char *sText, int indent) {
    char sIndent[16];
    char sIndent2[16];

    if (indent != -1)
    {
        sprintf(sIndent, "\n%%%ds", indent);
        sprintf(sIndent2, "%%%ds", indent+2);
        buf_printf(b, sIndent, " ");
        buf_printf(b, "%s\n", sText);
        buf_printf(b, sIndent2, " ");
    }
}

void do_node2s(Buffer *b, Node *node, int indent) {
    char sIndent[16];
    char sIndent2[16];
    int  nextIndent;

    if (indent == -1)
    {
        strcpy(sIndent, "");
        strcpy(sIndent2, "");
        nextIndent = -1;
    } else
    {
        sprintf(sIndent, "\n%%%ds", indent);
        sprintf(sIndent2, "%%%ds", indent+2);
        nextIndent = indent+2;
    }
    if (!node) {
        buf_printf(b, "(nil)");
        return;
    }
    switch (node->kind) {
    case AST_LITERAL:
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "AST_LITERAL\n");
            buf_printf(b, sIndent2, " ");
        }
        switch (node->ty->kind) {
        case KIND_CHAR:
            if (node->ival >= ' ' && node->ival <= '~')
            {
              buf_printf(b, "'%c'", node->ival);
              break;
            }
        case KIND_INT:
            buf_printf(b, "%d", node->ival);
            break;
        case KIND_LONG:
            buf_printf(b, "%ldL", node->ival);
            break;
        case KIND_LLONG:
            buf_printf(b, "%lldL", node->ival);
            break;
        case KIND_FLOAT:
        case KIND_DOUBLE:
        case KIND_LDOUBLE:
            buf_printf(b, "%f", node->fval);
            break;
        case KIND_ARRAY:
            buf_printf(b, "\"%s\"", quote_cstring(node->sval));
            break;
        default:
            error("internal error");
        }
        break;
    case AST_LABEL:
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "AST_LABEL\n");
            buf_printf(b, sIndent2, " ");
        }
        buf_printf(b, "%s:", node->label);
        break;
    case AST_LVAR:
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "AST_LVAR\n");
            buf_printf(b, sIndent2, " ");
        }

        buf_printf(b, "lv=%s", node->varname);
        if (node->lvarinit) {
            buf_printf(b, "(");
            a2s_declinit(b, node->lvarinit, nextIndent);
            buf_printf(b, ")");
        }
        break;
    case AST_GVAR:
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "AST_GVAR\n");
            buf_printf(b, sIndent2, " ");
        }

        buf_printf(b, "gv=%s", node->varname);
        break;
    case AST_FUNCALL:
    case AST_FUNCPTR_CALL: {
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "AST_FUNCALL\n");
            buf_printf(b, sIndent2, " ");
        }

        buf_printf(b, "(%s)%s(", ty2s(node->ty),
                   node->kind == AST_FUNCALL ? node->fname : node2s(node, nextIndent));
        for (int i = 0; i < vec_len(node->args); i++) {
            if (i > 0)
                buf_printf(b, ",");
            buf_printf(b, "%s", node2s(vec_get(node->args, i), nextIndent));
        }
        if (indent != -1)
        {
            buf_printf(b, "\n");
            buf_printf(b, sIndent2, " ");
        }
        buf_printf(b, ")");
        break;
    }
    case AST_FUNCDESG: {
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "AST_FUNCDESG\n");
            buf_printf(b, sIndent2, " ");
        }

        buf_printf(b, "(funcdesg %s)", node->fname);
        break;
    }
    case AST_FUNC: {
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "AST_FUNC\n");
            buf_printf(b, sIndent2, " ");
        }
        buf_printf(b, "(%s)%s(", ty2s(node->ty), node->fname);
        for (int i = 0; i < vec_len(node->params); i++) {
            if (i > 0)
                buf_printf(b, ",");
            Node *param = vec_get(node->params, i);
            buf_printf(b, "%s (%s)", node2s(param, nextIndent), ty2s(param->ty));
        }
        if (indent != -1)
        {
            buf_printf(b, "\n");
            buf_printf(b, sIndent2, " ");
        }
        buf_printf(b, ")");
        do_node2s(b, node->body, nextIndent);
        break;
    }
    case AST_GOTO:
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "AST_GOTO\n");
            buf_printf(b, sIndent2, " ");
        }

        buf_printf(b, "goto(%s)", node->label);
        break;
    case AST_DECL:
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "AST_DECL\n");
            buf_printf(b, sIndent2, " ");
        }

        buf_printf(b, "(decl %s%s %s",
                   ty2class(node->declvar->ty),
                   ty2s(node->declvar->ty),
                   node->declvar->varname);
        if (node->declinit) {
            buf_printf(b, " ");
            a2s_declinit(b, node->declinit, nextIndent);
        }
        if (indent != -1)
        {
            buf_printf(b, "\n");
            buf_printf(b, sIndent2, " ");
        }
        buf_printf(b, ")");
        break;
    case AST_INIT:
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            //buf_printf(b, "AST_INIT\n");
            //buf_printf(b, sIndent2, " ");
            buf_printf(b, "AST_INIT");
            //buf_printf(b, sIndent2, " ");
        }

        buf_printf(b, "%s@%d", node2s(node->initval, nextIndent), node->initoff, ty2s(node->totype));
        break;
    case AST_CONV:
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "AST_CONV\n");
            buf_printf(b, sIndent2, " ");
            nextIndent += 2;
        }

        buf_printf(b, "(conv %s", node2s(node->operand, nextIndent));
        if (indent != -1)
        {
            buf_printf(b, "\n");
            buf_printf(b, sIndent2, " ");
        }
        buf_printf(b, "=>%s", ty2s(node->ty));
        if (indent != -1)
        {
            buf_printf(b, "\n");
            buf_printf(b, sIndent2, " ");
        }
        buf_printf(b, ")");
        break;
    case AST_IF:
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "AST_IF\n");
            buf_printf(b, sIndent2, " ");
            nextIndent += 2;
        }

        buf_printf(b, "(if %s %s",
                   node2s(node->cond, nextIndent),
                   node2s(node->then, nextIndent));
        if (node->els)
            buf_printf(b, " %s", node2s(node->els, nextIndent));
        if (indent != -1)
        {
            buf_printf(b, "\n");
            buf_printf(b, sIndent2, " ");
        }
        buf_printf(b, ")");
        break;
    case AST_TERNARY:
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "AST_TERNARY\n");
            buf_printf(b, sIndent2, " ");
        }

        buf_printf(b, "(? %s %s %s)",
                   node2s(node->cond, nextIndent),
                   node2s(node->then, nextIndent),
                   node2s(node->els, nextIndent));
        break;
    case AST_RETURN:
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "AST_RETURN\n");
            buf_printf(b, sIndent2, " ");
            nextIndent += 2;
        }

        buf_printf(b, "(return %s", node2s(node->retval, nextIndent));
        if (indent != -1)
        {
            buf_printf(b, "\n");
            buf_printf(b, sIndent2, " ");
        }
        buf_printf(b, ")");
        break;
    case AST_COMPOUND_STMT: {
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "AST_COMPOUND_STMT\n");
            buf_printf(b, sIndent2, " ");
            nextIndent += 2;
        }

        buf_printf(b, "{");
        for (int i = 0; i < vec_len(node->stmts); i++) {
            do_node2s(b, vec_get(node->stmts, i), nextIndent);
            buf_printf(b, ";");
        }
        if (indent != -1)
        {
            buf_printf(b, "\n");
            buf_printf(b, sIndent2, " ");
        }
        buf_printf(b, "}");
        break;
    }
    case AST_STRUCT_REF:
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "AST_STRUCT_REF");
            //buf_printf(b, sIndent2, " ");
        }

        do_node2s(b, node->struc, nextIndent);
        buf_printf(b, ".");
        buf_printf(b, node->field);
        break;
    case AST_ADDR:  do_ast(b, "AST_ADDR", indent); uop_to_string(b, "addr", node, nextIndent); break;
    case AST_DEREF: do_ast(b, "AST_DEREF", indent); uop_to_string(b, "deref", node, nextIndent); break;
    case OP_SAL:  do_ast(b, "AST_OP_SAL", indent); binop_to_string(b, "<<", node, nextIndent); break;
    case OP_SAR:
    case OP_SHR:  do_ast(b, "AST_OP_SHR", indent); binop_to_string(b, ">>", node, nextIndent); break;
    case OP_GE:  do_ast(b, "AST_OP_GE", indent); binop_to_string(b, ">=", node, nextIndent); break;
    case OP_LE:  do_ast(b, "AST_OP_LE", indent); binop_to_string(b, "<=", node, nextIndent); break;
    case OP_NE:  do_ast(b, "AST_OP_NE", indent); binop_to_string(b, "!=", node, nextIndent); break;
    case OP_PRE_INC: do_ast(b, "AST_OP_PRE_INC", indent); uop_to_string(b, "pre++", node, nextIndent); break;
    case OP_PRE_DEC: do_ast(b, "AST_OP_PRE_DEC", indent); uop_to_string(b, "pre--", node, nextIndent); break;
    case OP_POST_INC: do_ast(b, "AST_OP_POST_INC", indent); uop_to_string(b, "post++", node, nextIndent); break;
    case OP_POST_DEC: do_ast(b, "AST_OP_POST_DEC", indent); uop_to_string(b, "post--", node, nextIndent); break;
    case OP_LOGAND: do_ast(b, "AST_OP_LOGAND", indent); binop_to_string(b, "and", node, nextIndent); break;
    case OP_LOGOR:  do_ast(b, "AST_OP_LOGOR", indent); binop_to_string(b, "or", node, nextIndent); break;
    case OP_A_ADD:  do_ast(b, "AST_OP_A_ADD", indent); binop_to_string(b, "+=", node, nextIndent); break;
    case OP_A_SUB:  do_ast(b, "AST_OP_A_SUB", indent); binop_to_string(b, "-=", node, nextIndent); break;
    case OP_A_MUL:  do_ast(b, "AST_OP_A_MUL", indent); binop_to_string(b, "*=", node, nextIndent); break;
    case OP_A_DIV:  do_ast(b, "AST_OP_A_DIV", indent); binop_to_string(b, "/=", node, nextIndent); break;
    case OP_A_MOD:  do_ast(b, "AST_OP_A_MOD", indent); binop_to_string(b, "%=", node, nextIndent); break;
    case OP_A_AND:  do_ast(b, "AST_OP_A_AND", indent); binop_to_string(b, "&=", node, nextIndent); break;
    case OP_A_OR:   do_ast(b, "AST_OP_A_OR", indent); binop_to_string(b, "|=", node, nextIndent); break;
    case OP_A_XOR:  do_ast(b, "AST_OP_A_XOR", indent); binop_to_string(b, "^=", node, nextIndent); break;
    case OP_A_SAL:  do_ast(b, "AST_OP_A_SAL", indent); binop_to_string(b, "<<=", node, nextIndent); break;
    case OP_A_SAR:
    case OP_A_SHR:  do_ast(b, "AST_OP_A_SHR", indent); binop_to_string(b, ">>=", node, nextIndent); break;
    case '!': do_ast(b, "AST_NOT", indent); uop_to_string(b, "!", node, nextIndent); break;
    case '&': do_ast(b, "AST_BITAND", indent); binop_to_string(b, "&", node, nextIndent); break;
    case '|': do_ast(b, "AST_BITOR", indent); binop_to_string(b, "|", node, nextIndent); break;
    case AST_PRUNED: break;
    case OP_CAST: {
        buf_printf(b, sIndent, " ");
        buf_printf(b, "OP_CAST\n");
        buf_printf(b, sIndent2, " ");

        buf_printf(b, "((%s)=>(%s) %s)",
                   ty2s(node->operand->ty),
                   ty2s(node->ty),
                   node2s(node->operand, nextIndent));
        break;
    }
    case OP_LABEL_ADDR:
        if (indent != -1)
        {
            buf_printf(b, sIndent, " ");
            buf_printf(b, "OP_LABEL_ADDR\n");
            buf_printf(b, sIndent2, " ");
        }
        buf_printf(b, "&&%s", node->label);
        break;
    default: {
        if (indent != -1)
        {
            switch (node->kind)
            {
              case OP_EQ: do_ast(b, "AST_OP_EQ", indent); break;
              case '+':   do_ast(b, "AST_OP_PLUS", indent); break;
              case '-':   do_ast(b, "AST_OP_MINUS", indent); break;
              case '*':   do_ast(b, "AST_OP_MULTIPLY", indent); break;
              case '/':   do_ast(b, "AST_OP_DIVIDE", indent); break;
              case '=':   do_ast(b, "AST_OP_ASSIGN", indent); break;
                        
              default:
                buf_printf(b, sIndent, " ");
                buf_printf(b, "AST_DEFAULT\n");
                buf_printf(b, sIndent2, " ");
                break;
            }
            nextIndent += 2;
        }

        char *left = node2s(node->left, nextIndent);
        char *right = node2s(node->right, nextIndent);
        if (node->kind == OP_EQ)
            buf_printf(b, "(== ");
        else
            buf_printf(b, "(%c ", node->kind);
        buf_printf(b, "%s %s", left, right);
        if (indent != -1)
        {
            buf_printf(b, "\n");
            buf_printf(b, sIndent2, " ");
        }
        buf_printf(b, ")", left, right);
    }
    }
}

char *node2s(Node *node, int indent) {
    Buffer *b = make_buffer();
    do_node2s(b, node, indent);
    return buf_body(b);
}

static char *encoding_prefix(int enc) {
    switch (enc) {
    case ENC_CHAR16: return "u";
    case ENC_CHAR32: return "U";
    case ENC_UTF8:   return "u8";
    case ENC_WCHAR:  return "L";
    }
    return "";
}

char *tok2s(Token *tok) {
    if (!tok)
        return "(null)";
    switch (tok->kind) {
    case TIDENT:
        return tok->sval;
    case TKEYWORD:
        switch (tok->id) {
#define op(id, str)         case id: return str;
#define keyword(id, str, _) case id: return str;
#include "keyword.inc"
#undef keyword
#undef op
        default: return format("%c", tok->id);
        }
    case TCHAR:
        return format("%s'%s'",
                      encoding_prefix(tok->enc),
                      quote_char(tok->c));
    case TNUMBER:
        return tok->sval;
    case TSTRING:
        return format("%s\"%s\"",
                      encoding_prefix(tok->enc),
                      quote_cstring(tok->sval));
    case TEOF:
        return "(eof)";
    case TINVALID:
        return format("%c", tok->c);
    case TNEWLINE:
        return "(newline)";
    case TSPACE:
        return "(space)";
    case TMACRO_PARAM:
        return "(macro-param)";
    }
    error("internal error: unknown token kind: %d", tok->kind);
}
