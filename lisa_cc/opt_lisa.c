// Copyright 2019 Ken Pettit <pettitkd@gmail.com>
// Releaed under the MIT license.

// Optimizations to the Abstract Syntax Tree for the
// Pico16 soft processor.

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "lisacc.h"

typedef void (*node_op_t)(Node *v, Node **vsource, int *changes,
              int parentAssignChar, Node *vNextSibling);

int gConvCount       = 0;
int gConvPruned      = 0;
int gTotalAstChanges = 0;

/*
======================================================================
Get type of node by name for debugging
======================================================================
*/
#if INCLUDE_NODE_DEBUG
static const char * GetNodeName(Node *v)
{
  const char *pStr;

  switch(v->kind)
  {
    case AST_FUNC          : pStr = "AST_FUNC          "; break;
    case AST_FUNCALL       : pStr = "AST_FUNCALL       "; break;
    case AST_FUNCPTR_CALL  : pStr = "AST_FUNCPTR_CALL  "; break;
    case AST_CONV          : pStr = "AST_CONV          "; break;
    case AST_INIT          : pStr = "AST_INIT          "; break;
    case AST_IF            : pStr = "AST_IF            "; break;
    case AST_TERNARY       : pStr = "AST_TERNARY       "; break;
    case AST_RETURN        : pStr = "AST_RETURN        "; break;
    case AST_COMPOUND_STMT : pStr = "AST_COMPOUND_STMT "; break;
    case AST_ADDR          : pStr = "AST_ADDR          "; break;
    case AST_DEREF         : pStr = "AST_DEREF         "; break;
    case OP_PRE_INC        : pStr = "OP_PRE_INC        "; break;
    case OP_PRE_DEC        : pStr = "OP_PRE_DEC        "; break;
    case OP_POST_INC       : pStr = "OP_POST_INC       "; break;
    case OP_POST_DEC       : pStr = "OP_POST_DEC       "; break;
    case OP_CAST           : pStr = "OP_CAST           "; break;
    case OP_SAL            : pStr = "OP_SAL            "; break;
    case OP_SAR            : pStr = "OP_SAR            "; break;
    case OP_SHR            : pStr = "OP_SHR            "; break;
    case OP_GE             : pStr = "OP_GE             "; break;
    case OP_LE             : pStr = "OP_LE             "; break;
    case OP_NE             : pStr = "OP_NE             "; break;
    case OP_LOGAND         : pStr = "OP_LOGAND         "; break;
    case OP_LOGOR          : pStr = "OP_LOGOR          "; break;
    case OP_A_ADD          : pStr = "OP_A_ADD          "; break;
    case OP_A_SUB          : pStr = "OP_A_SUB          "; break;
    case OP_A_MUL          : pStr = "OP_A_MUL          "; break;
    case OP_A_DIV          : pStr = "OP_A_DIV          "; break;
    case OP_A_MOD          : pStr = "OP_A_MOD          "; break;
    case OP_A_AND          : pStr = "OP_A_AND          "; break;
    case OP_A_OR           : pStr = "OP_A_OR           "; break;
    case OP_A_XOR          : pStr = "OP_A_XOR          "; break;
    case OP_A_SAL          : pStr = "OP_A_SAL          "; break;
    case OP_A_SAR          : pStr = "OP_A_SAR          "; break;
    case OP_A_SHR          : pStr = "OP_A_SHR          "; break;
    case OP_EQ             : pStr = "OP_EQ             "; break;
    default:                 pStr = "AST_UNKNOWN"; break;
  }

  return pStr;
}
#endif

/*
======================================================================
Optimize the ordering of global variables based on alignment to 
achieve best memory usages.
======================================================================
*/
static void OptimizeGlobalVarOrder(Vector *toplevels)
{
}

/*
======================================================================
Iterate over nodes searching for specific type
======================================================================
*/

static void IterateNodeSearch(Node *v, Node **vsource,
              node_op_t pFunc, int *changes, int parentAssignChar,
              Node* vNextSibling)
{
  int   i;

  if (!v)
    return;

  /* Call the function handler */
  (*pFunc)(v, vsource, changes, parentAssignChar, vNextSibling); 

  /* Iterate to node children */
  switch (v->kind)
  {
    case AST_FUNC:
      IterateNodeSearch(v->body, &v->body, pFunc, changes, parentAssignChar, NULL);
      break;

    case AST_FUNCALL:
    case AST_FUNCPTR_CALL:
        for (i = 0; i < vec_len(v->args); i++)
          IterateNodeSearch(vec_get(v->args,i), (Node **) &v->args->body[i], pFunc, changes, parentAssignChar, NULL);
        break;

    case AST_CONV:
    case AST_INIT:
      IterateNodeSearch(v->operand, &v->operand, pFunc, changes, parentAssignChar, NULL);
      break;

    case AST_DECL:
      if (v->declinit)
      {
         for (i = 0; i < vec_len(v->declinit); i++)
           IterateNodeSearch(vec_get(v->declinit,i), (Node **) &v->declinit->body[i], pFunc, changes, parentAssignChar, NULL);
      }
      break;

    case AST_IF:
    case AST_TERNARY:
      IterateNodeSearch(v->cond, &v->cond, pFunc, changes, parentAssignChar, NULL);
      IterateNodeSearch(v->then, &v->then, pFunc, changes, parentAssignChar, NULL);
      IterateNodeSearch(v->els, &v->els, pFunc, changes, parentAssignChar, NULL);
      break;

    case AST_STRUCT_REF:
      IterateNodeSearch(v->struc, &v->struc, pFunc, changes, parentAssignChar, NULL);
      break;

    case AST_RETURN:
      IterateNodeSearch(v->retval, &v->retval, pFunc, changes, v->retval->ty->kind == KIND_CHAR, NULL);
      break;

    case AST_COMPOUND_STMT:
      for (i = 0; i < vec_len(v->stmts); i++)
      {
        Node *sibling;
        if (i < vec_len(v->stmts)-1)
          sibling = vec_get(v->stmts, i+1);
        else
          sibling = NULL;
        IterateNodeSearch(vec_get(v->stmts,i), (Node**) &v->stmts->body[i], pFunc, changes, parentAssignChar, sibling);
      }
      break;

    case AST_ADDR:
    case AST_DEREF:
    case OP_PRE_INC:
    case OP_PRE_DEC:
    case OP_POST_INC:
    case OP_POST_DEC:
    case OP_CAST:
    case '!':
      IterateNodeSearch(v->operand, &v->operand, pFunc, changes, parentAssignChar, NULL);
      break;

    case '&':
    case '|':
    case OP_SAL:
    case OP_A_SAL:
    case OP_SAR:
    case OP_SHR:
    case OP_GE:
    case OP_LE:
    case OP_NE:
    case OP_LOGAND:
    case OP_LOGOR:
    case OP_A_ADD:
    case OP_A_SUB:
    case OP_A_MUL:
    case OP_A_DIV:
    case OP_A_MOD:
    case OP_A_AND:
    case OP_A_OR:
    case OP_A_XOR:
    case OP_A_SAR:
    case OP_A_SHR:
    case OP_EQ:
    case '*':
    case '/':
    case '^':
    case '%':
    case '<':
    case '>':
    case '-':
    case '+':
      IterateNodeSearch(v->left, &v->left, pFunc, changes, parentAssignChar, NULL);
      IterateNodeSearch(v->right, &v->right, pFunc, changes, parentAssignChar, NULL);
      break;

    case '=':
      IterateNodeSearch(v->left, &v->left, pFunc, changes, parentAssignChar, NULL);
      IterateNodeSearch(v->right, &v->right, pFunc, changes,
          (v->left->ty->kind == KIND_SHORT || v->left->ty->kind == KIND_INT) ? 2 :
          v->left->ty->kind == KIND_CHAR ? 1 : 0, NULL);
      break;
  }
}

/*
======================================================================
Prune AST_CONV nodes thare are children of AST_NOT
======================================================================
*/
static void PruneNotConv(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  Node  *v2 = v->operand->operand;
  Type  *ty = v2->ty;

  if (parentAssignChar == 2)
      return;

  if (ty && ty->size == 1)
  {
    // Prune the conv node
    v->operand = v2;
    v->ty = ty;
    (*changes)++;
  }
}

/*
======================================================================
Prune AST_CONV nodes to KIND_SHORT or KIND_INT.
======================================================================
*/
static void PruneConvShortInt(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  Node  *v2 = v->operand;
  Type  *ty = v->operand->ty;
  int   kind = v2->kind;

  /* Test for struct reference */
  if (v2->kind == AST_STRUCT_REF)
  {
    ty = dict_get(v2->struc->ty->fields, v2->field); 
    kind = v2->struc->kind;
    //v2 = v2->struc;
  }

  /* Test the operand type */
  switch (kind)
  {
    case AST_LITERAL:
      /* Test for conversions with no effect */
      if ((ty->kind == KIND_CHAR ||
           ty->kind == KIND_INT) ||
          (ty->kind == KIND_LONG && v2->ival <32768 &&
           v2->ival > -32769))
      {
        /* Replace the AST_CONV Node with it's operand node */
        *vsource = v2;
        gConvPruned++;
        *changes += 1;
        return;
      } 
      break;

    case AST_LVAR:
    case AST_GVAR:
      /* Test for converstion from uchar to int / short */
      if (ty)
      {
        if ((ty->kind == KIND_CHAR && ty->usig) ||
            (ty->kind == v->ty->kind &&
             ty->usig == v->ty->usig))
        {
          /* Replace the AST_CONV Node with it's operand node */
          *vsource = v2;
          *changes += 1;
          gConvPruned++;
          return;
        }
      }
      break;

    case AST_DEREF:
      if (v2->operand->kind == '+')
      {
        Node* pOp = v2->operand;

        // char array deref gets a cast to int conv.  Remove it
        if (pOp->operand->kind == AST_CONV && 
            pOp->operand->operand->ty->kind == KIND_ARRAY &&
            pOp->operand->operand->ty->align == 1)
        {
          *vsource = v2;
          *changes += 1;
          gConvPruned++;
          return;
        }
      }
      break;

    default:
      break;
    }
}

/*
======================================================================
Prune AST_CONV nodes to KIND_CHAR.
======================================================================
*/
static void PruneConvChar(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  Node  *v2 = v->operand;
  Type  *ty = v2->ty;

  if (parentAssignChar == 2)
      return;

  /* Test the operand type */
  switch (v2->kind)
  {
    case AST_CONV:
      /* Test for dereference assignment from uchar to uchar.
       * the parser converts the dereference to int then 
       * back to uchar.  Let's undo that.
       */
      if (ty && (ty->kind == KIND_SHORT ||
                             ty->kind == KIND_INT) &&
          v2->operand->kind == AST_DEREF)
      {
        Node *var;

        /* Get the variable dereference */
        var = v2->operand->operand;
        
        /* Test for POST_INC, PRE_INC, etc. */
        if (var->kind == OP_PRE_INC || var->kind == OP_POST_INC ||
            var->kind == OP_PRE_DEC || var->kind == OP_POST_DEC)
        {
          var = var->operand;
        }

        /* Test the variable type */
        if (var->ty && v->ty->kind == KIND_CHAR)
        {
          /* Replace the AST_CONST Node with the AST_DEREF node */
          *vsource = v2->operand;
          gConvPruned++;
          *changes += 2;
        }
      }
      else if (ty && (ty->kind == KIND_SHORT ||
                                  ty->kind == KIND_INT) &&
          v2->operand->kind == AST_STRUCT_REF)
      {
        Type *ty2;

        /* Get the struct member type */
        ty2 = dict_get(v2->operand->struc->ty->fields,
                      v2->operand->field); 
        if (ty2 && ty2->kind == KIND_CHAR)
        {
          /* Prune the double AST_CONV and replace directly
           * with the AST_STRUCT_REF node.
           */
          *vsource = v2->operand;
          gConvPruned++;
          *changes += 1;
        }
      }
      else if (ty && ty->kind == KIND_CHAR)
      {
          /* Prune the double AST_CONV and replace directly
           * with single AST_CONV node.
           */
          *vsource = v2->operand;
          gConvPruned++;
          *changes += 1;
      }
      if (ty && ty->kind == KIND_INT)
      {
          /* Prune the double AST_CONV and replace directly
           * with the AST_STRUCT_REF node.
           */
          *vsource = v2->operand;
          gConvPruned++;
          *changes += 1;
      }
      break;

    /* Test for char to uchar or uchar to char conversion */
    case AST_LVAR:
    case AST_GVAR:
//    case '+':
//    case '-':
//    case '*':
    case '^':
    case '&':
    case '|':
    case OP_SAL:
    case OP_SAR:
    case OP_SHR:
    case OP_A_SAL:
    case OP_A_SAR:
    case OP_A_SHR:
      if (ty->kind == KIND_CHAR)
      {
        /* Remove the AST_CONV node */
        *vsource = v2;
        gConvPruned++;
        *changes += 1;
      }
      break;

    case '<':
    case '>':
    case '!':
    case OP_GE:
    case OP_LE:
      /* Remove the AST_CONV node */
      *vsource = v2;
      gConvPruned++;
      *changes += 1;
      break;

    case '+':
      if (v2->left->ty->kind == KIND_CHAR && v2->right->ty->kind == KIND_CHAR &&
          v2->left->ty->usig == v2->right->ty->usig)
      {
        *vsource = v2;
        gConvPruned++;
        *changes += 1;
      }
      break;

    case AST_LITERAL:
      /* Test for conversions with no effect */
      if ((ty->kind == KIND_CHAR ||
           ty->kind == KIND_INT ||
           ty->kind == KIND_LONG) &&
           v2->ival <= 255 && v2->ival >= 0)
      {
        /* Replace the AST_CONV Node with it's operand node */
        *vsource = v2;
        ty->kind = KIND_CHAR;
        ty->usig = true;
        gConvPruned++;
        *changes += 1;
        return;
      } 
      /* Test for conversions with no effect */
      else if ((ty->kind == KIND_CHAR ||
           ty->kind == KIND_INT ||
           ty->kind == KIND_LONG) &&
           v2->ival <= 127 && v2->ival >= -128)
      {
        /* Replace the AST_CONV Node with it's operand node */
        *vsource = v2;
        ty->kind = KIND_CHAR;
        ty->size = 1;
        gConvPruned++;
        *changes += 1;
        return;
      } 
      break;
  }
}

/*
======================================================================
Prune AST_CONV nodes to KIND_PTR.
======================================================================
*/
#if 0
static void PruneConvPtr(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  /* Test for char * type */
  if (v->ty->ptr->kind == KIND_CHAR && 
      v->operand->kind == AST_LITERAL &&
      v->operand->ty->kind == KIND_ARRAY)
  {
    /* Replace the AST_CONST Node with the AST_LITERAL node */
    *vsource = v->operand;
    (*changes)++;
    gConvPruned++;
  }
}
#endif

/*
======================================================================
Optimize out un-needed AST_CONV nodes.
======================================================================
*/
static void PruneConvNodes(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  /* Only process AST_CONV nodes */
  if ((v->kind != AST_CONV) &&
      !((v->kind == '!' || v->kind == '~') && v->operand->kind == AST_CONV))
    return;

  gConvCount++;

  /* Test for CONV to int of AST_NOT */
  if (v->kind == '!' || v->kind == '~')
    PruneNotConv(v, vsource, changes, parentAssignChar, vNextSibling);

  /* Test for conversion to short or int */
  else if (v->ty && (v->ty->kind == KIND_SHORT || v->ty->kind == KIND_INT))
    PruneConvShortInt(v, vsource, changes, parentAssignChar, vNextSibling);

  else if (v->ty && v->ty->kind == KIND_CHAR)
    PruneConvChar(v, vsource, changes, parentAssignChar, vNextSibling);

  /* Test for conversion to ptr */
//  else if (v->ty && v->ty->kind == KIND_PTR)
//    PruneConvPtr(v, vsource, changes);
}

/*
======================================================================
Optimize out un-needed AST_CONV nodes associated with AST_RETURN.
======================================================================
*/
static void PruneReturnConvNodes(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  /* Only process AST_RETURN nodes */
  if (v->kind != AST_RETURN)
    return;

  if (v->retval->kind != AST_CONV)
    return;

  gConvCount++;

  /* Test for unneeded conversion */
  if (v->retval->ty->kind == v->retval->operand->ty->kind)
  {
    v->retval = v->retval->operand;
    (*changes)++;
  }
}

/*
======================================================================
Optimize out assign to variable followed by return of that variable
======================================================================
*/
static void PruneAssignReturn(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  /* Only process AST_CONV nodes */
  if (v->kind != '=')
    return;

  if (vNextSibling == NULL)
    return;

  if (vNextSibling->kind != AST_RETURN)
    return;

  gConvCount++;

  /* Test that assign and return val are both LVAR */
  if (v->left->kind == AST_LVAR && 
      vNextSibling->retval->kind == AST_LVAR)
  {
    if (strcmp(v->left->varname, vNextSibling->retval->varname) == 0)
    {
      /* Convert v to an AST_RETURN of it's right assign value */
      v->kind = AST_RETURN;
      v->ty = v->right->ty;
      v->retval = v->right;

      /* Make the sibling AST_RETURN a NOP */
      vNextSibling->kind = AST_PRUNED;
    }
  }
}

/*
======================================================================
Optimize out assign to variable followed by return of that variable
======================================================================
*/
static void PruneConstArrayAdd(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  /* Only process AST_DEREF nodes with operand '+' */
  if (v->kind != AST_DEREF)
    return;
  if (v->operand->kind != '+')
    return;

  /* Test if right node is literal */
  if (v->operand->right->kind != AST_LITERAL ||
      v->operand->left->kind != AST_CONV)
  {
    return;
  }

  /* Test if literal is less than 128 */
  if (v->operand->right->ival >= 128 || v->operand->right->ival < 0)
    return;

  /* Test if AST_CONV ty->kind is array */
  if (v->operand->left->ty->kind != KIND_PTR)
    return;

  /* Make the literal value an offset to the AST_CONV and prune the AST_OP_PLUS */
  v->operand->ty->size = v->operand->left->operand->ty->align;
  v->operand->left->ty->offset = v->operand->right->ival * v->operand->ty->size;
  v->operand->left->ty->size = v->operand->left->operand->ty->align;
  v->operand = v->operand->left;
  (*changes)++;
}

/*
======================================================================
Optimize x = x + 1 to x++
======================================================================
*/
static void OptimizePostInc(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  // Skip if assigning to int
  if (parentAssignChar == 2)
    return;

  // Only operate on ASSIGN 
  if (v->kind != '=')
    return;

  // Right assignment will be CONV from PLUS
  if (v->right->kind != AST_CONV)
    return;

  // Test if conversion type is '+'
  if (v->right->left->kind != '+')
    return;

  // Test if '+' left operator matches var type of v
  if (v->left->kind != v->right->left->left->kind)
    return;

  // Test varname
  if (strcmp(v->left->varname, v->right->left->left->varname) != 0)
    return; 

  // Test if adding literal
  if (v->right->left->right->kind != AST_LITERAL)
    return;

  // Test if adding 1
  if (v->right->left->right->ival == 1)
  {
    // Change the ASSIGN to a POSTINC
    v->kind = OP_POST_INC;
    (*changes)++;
  }
  // Test if adding -1
  else if (v->right->left->right->ival == -1)
  {
    // Change the ASSIGN to a POSTDEC
    v->kind = OP_POST_DEC;
    (*changes)++;
  }
}

/*
======================================================================
Optimize x = x - 1 to x--
======================================================================
*/
static void OptimizePostDec(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  // Skip if assigning to int
  if (parentAssignChar == 2)
    return;

  // Only operate on ASSIGN 
  if (v->kind != '=')
    return;

  // Right assignment will be CONV from PLUS
  if (v->right->kind != AST_CONV)
    return;

  // Test if conversion type is '-'
  if (v->right->left->kind != '-')
    return;

  // Test if '+' left operator matches var type of v
  if (v->left->kind != v->right->left->left->kind)
    return;

  // Test varname
  if (strcmp(v->left->varname, v->right->left->left->varname) != 0)
    return; 

  // Test if adding literal
  if (v->right->left->right->kind != AST_LITERAL)
    return;

  // Test if adding 1
  if (v->right->left->right->ival == 1)
  {
    // Change the ASSIGN to a POST_DEC
    v->kind = OP_POST_DEC;
    (*changes)++;
  }
  // Test if adding -1
  else if (v->right->left->right->ival == -1)
  {
    // Change the ASSIGN to a POST_INC
    v->kind = OP_POST_INC;
    (*changes)++;
  }
}

/*
======================================================================
Optimize operation size of << >> &|^ operations.
======================================================================
*/
static void OptimizeOperationSize(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  Node  *vl;
  Node  *vr;
  Type  *ty;
  Type  *tyl;
  Type  *tyr;

  if (parentAssignChar == 2)
    return;

  /* Test the operand type */
  switch (v->kind)
  {
    case '-':
    case '+':
      // Test for pointer arithmetic
      if (v->left->kind == AST_CONV && v->left->ty->kind == KIND_PTR)
      {
        if (v->right->kind == AST_LITERAL && v->right->ival < 256)
        {
          if (v->right->ty->kind != KIND_CHAR)
          {
            v->right->ty->kind = KIND_CHAR;
            (*changes)++;
          }
          if (v->right->ty->size != 1)
          {
            v->right->ty->size = 1;
            (*changes)++;
          }
        }
      }
      // Fall through

    case '*':
    case '^':
    case '&':
    case '|':
    case '>':
    case '<':
    case OP_GE:
    case OP_LE:
    case OP_SAL:
    case OP_SAR:
    case OP_SHR:
    case OP_A_SAL:
    case OP_A_SAR:
    case OP_A_SHR:
      ty = v->ty;
      vl = v->left;
      tyl = vl->ty;
      vr = v->right;
      tyr = vr->ty;

      if ((ty->kind != tyl->kind || ty->kind != tyr->kind) &&
          tyl->kind == tyr->kind)
      {
        ty->kind = tyl->kind;
        (*changes)++;
      }

      // Test for comparison between AST_CONV(int) and char LITERAL
      else if (vl->kind == AST_CONV &&
               vl->ty->kind == KIND_INT &&
               vl->operand->ty->kind == KIND_CHAR && 
               vr->kind == AST_LITERAL && 
               vr->ival >= -128 && vr->ival <= 255)
      {
        // Remove the AST_CONV to int on the left hand side
        v->left = vl->operand;
        v->ty   = vl->ty;
        (*changes)++;
      }

      // Test for right and left conv to int when both are CHAR
      else if (vl->kind == AST_CONV &&
               vl->ty->kind == KIND_INT &&
               vl->operand->ty->kind == KIND_CHAR && 
               vr->kind == AST_CONV && 
               vr->ty->kind == KIND_INT &&
               vr->operand->ty->kind == KIND_CHAR)
      {
        // Remove the AST_CONV to int on the left hand side
        v->left = vl->operand;
        v->ty   = vl->ty;
        v->right = vr->operand;
        (*changes)++;
      }
      break;

    case AST_LITERAL:
      if (v->ty->kind == KIND_CHAR &&
          v->ival <= 255 && v->ival >= 0)
      {
          /* Ensure the literal is marked as usig */
          v->ty->usig = true;
      }
      break;

    case AST_IF:
    case AST_TERNARY:
      if (v->cond->left->kind == AST_CONV ||
          (v->cond->right && v->cond->right->kind == AST_CONV))
      {
          /* Test for pointer deref conversion to int with char */
          if (v->cond->left->kind == AST_CONV)
          {
            vl = v->cond->left;
            vr = v->cond->right;
          }
          else
          {
            vl = v->cond->right;
            vr = v->cond->left;
          }
          
          /* Test if left is AST_CONV */
          if (vl->operand->kind == AST_DEREF &&
              vl->operand->ty->kind == KIND_CHAR &&
              vl->ty->kind != KIND_CHAR &&
              vr->kind == AST_LITERAL &&
              vr->ty->kind != KIND_CHAR &&
              vr->ival >= -128 && vr->ival <= 255)
          {
            /* Remove the AST_CONV */
            if (vl == v->cond->left)
              v->cond->left = v->cond->left->operand;
            else
              v->cond->right = v->cond->right->operand;

            /* Set the LITERAL to type Char */
            vr->ty->kind = KIND_CHAR;
            vr->ty->size = 1;
          }
      }
      break;

    case AST_STRUCT_REF:
      if (v->ty->size > 1 && v->operand->ty->size == 1)
      {
        v->ty->size = 1;
        v->ty->kind = KIND_CHAR;
        (*changes)++;
      }
      break;

    default:
      return;
  }
}
        
/*
======================================================================
Prune compiler time math operations
======================================================================
*/
static void PruneConstIntegerMath(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  int   lkind, rkind;

  /* Only process math operation nodes */
  if (v->kind != '+' && v->kind != '-' &&
      v->kind != '*' && v->kind != '/' &&
      v->kind != '%' && v->kind != OP_SAL &&
      v->kind != '^' && v->kind != OP_SAR &&
      v->kind != '|' && v->kind != '&')
    return;

  /* Test if both left and right hand side are AST_LITERAL */
  if (v->left->kind != AST_LITERAL || v->right->kind != AST_LITERAL)
    return;

  lkind = v->left->ty->kind;
  rkind = v->right->ty->kind;
  if (lkind != KIND_CHAR && lkind != KIND_SHORT && 
      lkind != KIND_INT  && lkind != KIND_LONG &&
      lkind != KIND_LLONG)
  {
    return;
  }

  if (rkind != KIND_CHAR && rkind != KIND_SHORT && 
      rkind != KIND_INT  && rkind != KIND_LONG &&
      rkind != KIND_LLONG)
  {
    return;
  }

  /* Get the resulting type */
  if (v->left->ty->kind > v->right->ty->kind)
    v->ty = v->left->ty;
  else
    v->ty = v->right->ty;

  /* Perform the math */
  switch (v->kind)
  {
    case '+':
      v->ival = v->left->ival + v->right->ival;
      break;

    case '-':
      v->ival = v->left->ival - v->right->ival;
      break;

    case '*':
      v->ival = v->left->ival * v->right->ival;
      break;

    case '/':
      v->ival = v->left->ival / v->right->ival;
      break;

    case '%':
      v->ival = v->left->ival % v->right->ival;
      break;

    case '^':
      v->ival = v->left->ival ^ v->right->ival;
      break;

    case '|':
      v->ival = v->left->ival | v->right->ival;
      break;

    case '&':
      v->ival = v->left->ival & v->right->ival;
      break;

    case OP_SAL:
      v->ival = v->left->ival << v->right->ival;
      break;

    case OP_SAR:
      v->ival = v->left->ival >> v->right->ival;
      break;
  }

  /* Change the operation to an AST_LITERAL */
  v->kind = AST_LITERAL;

  (*changes)++;
}

/*
======================================================================
Prune power of two divides -> convert to shift right
======================================================================
*/
static void PruneConstPowerOfTwoDivide(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  int   rkind, lkind;
  Node *right;

  /* Only process math operation nodes */
  if (v->kind != '/' && v->kind != '*')
    return;

  /* Test if and right hand side is AST_LITERAL */
  if (v->right->kind != AST_LITERAL && 
      !(v->right->kind == AST_CONV && v->right->operand->kind == AST_LITERAL))
    return;

  if (v->right->kind == AST_CONV)
    right = v->right->operand;
  else
    right = v->right;

  lkind = v->left->ty->kind;
  rkind = right->ty->kind;
  if (lkind != KIND_CHAR && lkind != KIND_SHORT && 
      lkind != KIND_INT  && lkind != KIND_LONG &&
      lkind != KIND_LLONG)
  {
    return;
  }

  if (rkind != KIND_CHAR && rkind != KIND_SHORT && 
      rkind != KIND_INT  && rkind != KIND_LONG &&
      rkind != KIND_LLONG)
  {
    return;
  }

  /* Test if right hand side is power of two */
  if (right->ival && (right->ival & (right->ival-1)) == 0)
  {
    /* Okay, it is a power of two.  Convert to a shift right */
    v->kind = v->kind == '/' ? OP_SHR : OP_SAL;
    right->ival = log2l(right->ival);

    /* Test for conv and remove it */
    if (v->right->kind == AST_CONV)
      v->right = right;
    (*changes)++;
  }
}

/*
======================================================================
Optimize size of literals for nodes with left and right operators
======================================================================
*/
static void OptimizeLiteralSizes(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  Node  *lit;
  Node  *other;

  switch (v->kind)
  {
    case '&':
    case '|':
    case OP_SAL:
    case OP_A_SAL:
    case OP_SAR:
    case OP_SHR:
    case OP_GE:
    case OP_LE:
    case OP_NE:
    case OP_LOGAND:
    case OP_LOGOR:
    case OP_A_ADD:
    case OP_A_SUB:
    case OP_A_MUL:
    case OP_A_DIV:
    case OP_A_MOD:
    case OP_A_AND:
    case OP_A_OR:
    case OP_A_XOR:
    case OP_A_SAR:
    case OP_A_SHR:
    case OP_EQ:
    case '*':
    case '/':
    case '=':
    case '^':
    case '%':
    case '<':
    case '>':
    case '-':
    case '+':
      break;

    default:
      return;
  }

#if 0
  /* Only process nodes with left and right sides */
  if (!v->left || !v->right)
    return;
#endif

  /* One side must be LITERAL */
  if (v->right->kind == AST_LITERAL)
  {
    lit = v->right;
    other = v->left;
  }
  else if (v->left->kind == AST_LITERAL)
  {
    lit = v->left;
    other = v->right;
  }
  else
    return;

  /* Don't optimizes ops that have two literals */
  if (other->kind == AST_LITERAL)
    return;

  /* Optimize for char size */
  if (other->ty && other->ty->kind == KIND_CHAR && lit->ty->kind != KIND_CHAR)
  {
    /* Test if the literal can be made char */
    if (lit->ival >= -128 && lit->ival <= 255)
    {
      lit->ty->kind = KIND_CHAR;
      lit->ty->size = 1;
      (*changes)++;
      return;
    }
  }
}

/*
======================================================================
Prune constant IF nodes
======================================================================
*/
static void PruneConstIfNodes(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  int     kind;
  bool    isTrue = 0;
  Node    *repl;

  /* Test for AST_IF node */
  if (v->kind != AST_IF)
    return;

  /* Test for AST_LITERAL as the the cond node */
  if (v->cond->kind == AST_LITERAL)
  {
    /* Test the literal value */
    kind = v->cond->ty->kind;
    if (kind == KIND_CHAR || kind == KIND_SHORT || kind == KIND_INT ||
        kind == KIND_LONG || kind == KIND_LLONG)
    {
      isTrue = v->cond->ival != 0;
    }
    else if (kind == KIND_FLOAT || kind == KIND_DOUBLE ||
             kind == KIND_LDOUBLE)
    {
      isTrue = v->cond->fval != 0.0;
    }
    else
      /* No pruning if not int or float types */
      return;
  }
  else
    return;

  /* Test which branch to replace with */
  if (isTrue)
    repl = v->then;
  else
    repl = v->els;

  /* Replace the AST_IF with the appropriate path */
  if (repl != NULL)
    *vsource = repl;
  else
    v->kind = AST_NOP;
  (*changes)++;
}

/*
======================================================================
Prune AST_COMPOUND_STMT nodes with only a single operation
======================================================================
*/
static void PruneCompoundStmt(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  /* Test for AST_IF node */
  if (v->kind != AST_COMPOUND_STMT)
    return;

  /* Test for statement vector with only a single entry */
  if (vec_len(v->stmts) == 1)
  {
    /* Replace the AST_COMPOUND_STMT node with it's one statement */
    *vsource = (Node *) v->stmts->body[0];
    (*changes)++;
  }
}

/*
======================================================================
Optimize AST_IF else condition if it is a single statement by 
making it the 'then' clause and reversing the if comparison.
======================================================================
*/
static void OptimizeIfElse(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  Node  *tmp;
  int   kind;

  /* Test for AST_IF node */
  if (v->kind != AST_IF && v->kind != AST_TERNARY)
    return;

  if (!v->cond->right || v->cond->right->kind == 0 ||
      v->cond->kind == AST_FUNCALL)
     return;

  /* Test for pointer post/pre inc optimization */
  if (v->cond->left->ty && v->cond->left->ty->kind == KIND_PTR && 
      v->cond->right && v->cond->right->ty->kind == KIND_PTR &&
      v->els == NULL &&
      (v->then->kind == OP_POST_INC ||
       v->then->kind == OP_POST_DEC))
  {
    if (strcmp(v->cond->left->varname, v->then->operand->varname) == 0)
    {
      // Swap left and right
      Node *temp = v->cond->left;
      v->cond->left = v->cond->right;
      v->cond->right = temp;
      (*changes)++;

      /* Reverse the IF condition kind */
      switch (v->cond->kind)
      {
        case '<': v->cond->kind = OP_GE; break;
        case '>': v->cond->kind = OP_LE; break;
        case OP_LE: v->cond->kind = '>'; break;
        case OP_GE: v->cond->kind = '<'; break;
      }
      return;
    }
  }

  if (v->els == NULL)
    return;

  /* Test for reversal based on the els type */
  switch (v->els->kind)
  {
    case AST_GOTO:
      break;

    case AST_RETURN:
      /* Test for simple return */
      if (v->els->retval == NULL)
        break;

      /* Test for return of AST_LITERAL */
      if (v->els->retval->kind == AST_LITERAL)
      {
        kind = v->els->retval->ty->kind;
        if ((kind == KIND_CHAR || kind == KIND_SHORT ||
             kind == KIND_INT  || kind == KIND_LONG ||
             kind == KIND_LLONG) && 
             v->els->retval->ival < 256 &&
             v->els->retval->ival >= 0)
        {
          break;
        }
        else
          return;
      }
      
      else if (v->els->retval->kind == AST_LVAR ||
          v->els->retval->kind == AST_GVAR)
        break;

      /* Complex return type ... not a single opcode */
      return;

    default:
      return;
  }

  /* Test the IF condition type */
  switch (v->cond->kind)
  {
    case '<':
    case '>':
    case OP_EQ:
    case OP_NE:
    case OP_LE:
    case OP_GE:
      break;
    
    default:
      return;
  }

  /* Reverse the 'then' and 'els' nodes */
  tmp = v->then;
  v->then = v->els;
  v->els = tmp;

  /* Reverse the IF condition kind */
  switch (v->cond->kind)
  {
    case '<': v->cond->kind = OP_GE; break;
    case '>': v->cond->kind = OP_LE; break;
    case OP_EQ: v->cond->kind = OP_NE; break;
    case OP_NE: v->cond->kind = OP_EQ; break;
    case OP_LE: v->cond->kind = '>'; break;
    case OP_GE: v->cond->kind = '<'; break;
  }
  (*changes)++;
}

/*
======================================================================
Check that nodes with KIND_BOOL or KIND_BYTE have size 1
======================================================================
*/
static void CheckByteNodeSizes(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  if (v->ty && (v->ty->kind == KIND_BOOL || v->ty->kind == KIND_CHAR) &&
      v->ty->size != 1)
  {
    v->ty->size = 1;
    (*changes)++;
  }
}

/*
======================================================================
Optimize AST_CONV to long for comparison against long AST_LITERAL
which has a value < 65536.
======================================================================
*/
static void OptimizeLongComparison(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  Node *left;
  Node *right;
  Node **pLeft;

  /* Test for comparision AST node */
  if (v->kind == '>'   || v->kind == '<' ||
      v->kind == OP_GE || v->kind == OP_LE ||
      v->kind == OP_NE || v->kind == OP_EQ)
  {
    /* Determine if Left or Right node are AST_CONV */
    if (v->left->kind == AST_CONV && v->left->ty->kind == KIND_LONG)
    {
      left = v->left;
      pLeft = &v->left;
      right = v->right;
    }
    else if (v->right->kind == AST_CONV && v->right->ty->kind == KIND_LONG)
    {
      left = v->right;
      pLeft = &v->right;
      right = v->left;
    }
    else
      return;

    /* Test if the right hand side is AST_LITERAL */
    if (right->kind == AST_LITERAL && right->ty->kind == KIND_LONG &&
        right->ival < 65536 && right->ival >= 0)
    {
      /* Convert the Literal to an int */
      right->ty->kind = KIND_INT;

      /* Remove the AST_CONV on the left hand side */
      *pLeft = left->operand;
      *changes += 1;
    }
    (*changes)++;
  }
}

/*
======================================================================
Optimize AST_CONV to int for comparison against long AST_LITERAL
which has a value < 256.
======================================================================
*/
static void OptimizeIntComparison(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  Node *left;
  Node *right;
  Node **pLeft;

  /* Test for comparision AST node */
  if (v->kind == '>'   || v->kind == '<' ||
      v->kind == OP_GE || v->kind == OP_LE ||
      v->kind == OP_NE || v->kind == OP_EQ)
  {
    /* Determine if Left or Right node are AST_CONV */
    if (v->left->kind == AST_CONV && v->left->ty->kind == KIND_INT)
    {
      left = v->left;
      pLeft = &v->left;
      right = v->right;
    }
    else if (v->right->kind == AST_CONV && v->right->ty->kind == KIND_INT)
    {
      left = v->right;
      pLeft = &v->right;
      right = v->left;
    }
    else
      return;

    /* Test if the right hand side is AST_LITERAL */
    if (right->kind == AST_LITERAL && right->ty->kind != KIND_CHAR &&
        right->ival < 255 && right->ival >= -128 &&
        left->operand->ty->kind == KIND_CHAR)
    {
      /* Convert the Literal to an int */
      right->ty->kind = KIND_CHAR;

      /* Remove the AST_CONV on the left hand side */
      *pLeft = left->operand;
      *changes += 1;
    }
  }
}

/*
======================================================================
Optimize associatve operations so simpler term is on the right. 
======================================================================
*/
static void OptimizeAssociatveArgs(Node *v, Node **vsource, int *changes, int parentAssignChar, Node* vNextSibling)
{
  Node *left;
  Node *right;

  /* Check for associatve operations */
  switch (v->kind)
  {
    case OP_EQ:
    case '|':
    case '&':
    case OP_LOGAND:
    case OP_LOGOR:
    case '+':
    case '*':
    case OP_A_AND:
    case OP_A_OR:
    case OP_A_XOR:
    case OP_NE:
    case '<':
    case '>':
    case OP_LE:
    case OP_GE:
      break;
    default:
      return;
  }

  /* Don't optimize pointer comparisons */
  if (v->right->ty->kind == KIND_PTR || v->left->ty->kind == KIND_PTR)
    return;

  left = v->left;
  right = v->left;
  if (left->kind == AST_LITERAL ||
      ((left->kind == AST_LVAR || left->kind == AST_GVAR) &&
      !(right->kind == AST_LVAR || right->kind == AST_GVAR)) ||
      (left->kind == AST_CONV &&
      (left->operand->kind == AST_LITERAL || left->operand->kind == AST_LVAR ||
          left->operand->kind == AST_GVAR)))
  {
    if (v->right->kind != AST_LITERAL && !(left->kind == right->kind) &&
        !((right->kind == AST_LVAR || right->kind == AST_GVAR) &&
          (left->kind == AST_LVAR || left->kind == AST_GVAR)))
    {
      // Switch left and right nodes
      v->left = v->right;
      v->right = left;

      // For < > <= >= we have to swap the comparison too
      if (v->kind == '<')
        v->kind = '>';
      else if (v->kind == '>')
        v->kind = '<';
      else if (v->kind == OP_LE)
        v->kind = OP_GE;
      else if (v->kind == OP_GE)
        v->kind = OP_LE;

      (*changes)++;
    }
  }
}
/*
======================================================================
Run an optimization over all nodes by iterating the optimization 
handler through the AST tree.
======================================================================
*/
static int RunOptimization(Vector *toplevels, node_op_t pFunc)
{
  int   i;
  int   changes;
  int   opt_changes = 0;
  Node  *v;

  /* Optimize out AST_CONV nodes that aren't needed */
  do
  {
     /* Initialize changes */
     changes = 0;

     /* Loop across all top level AST tree nodes */
     for (i = 0; i < vec_len(toplevels); i++)
       {
         v = vec_get(toplevels, i);
         Node *sibling;
         if (i < vec_len(toplevels)-1)
           sibling = vec_get(toplevels, i+1);
         else
           sibling = NULL;
         IterateNodeSearch(v, NULL, pFunc, &changes, 0, sibling);
       }

     /* Keep track of the number of changes we make to the AST tree */
     gTotalAstChanges += changes;
     opt_changes += changes;
  } while (changes > 0);

  /* report the number of changes for this optimization */
  return opt_changes;
}

/*
======================================================================
Run all known optimizations passes on the AST top level
======================================================================
*/
void LisaOptimizeAST(Vector *toplevels)
{
  int changes;
  int totalChanges = 0;

  /* Optimize global variable order */
  OptimizeGlobalVarOrder(toplevels);

  do 
  {
    /* Run the Constant integer math pruning optimization */
    changes = RunOptimization(toplevels, &PruneConstIntegerMath);
    
    /* Run the Constant power of 2 integer divide pruning optimization */
    changes = RunOptimization(toplevels, &PruneConstPowerOfTwoDivide);
    
    /* Run the Literal size optimization */
    changes = RunOptimization(toplevels, &OptimizeLiteralSizes);
    
    /* Run the AST_CONV pruning optimization */
    changes += RunOptimization(toplevels, &PruneConvNodes);
    
    /* Run the AST_CONV pruning optimization */
    changes += RunOptimization(toplevels, &PruneConstIfNodes);
    
    /* Run the AST_COMPOUND_STMT pruning optimization */
    changes += RunOptimization(toplevels, &PruneCompoundStmt);
    
    /* Run the AST_COMPOUND_STMT pruning optimization */
    changes += RunOptimization(toplevels, &OptimizeIfElse);

    /* Run optimization to set >> << |&^ operation sizes */
    changes += RunOptimization(toplevels, &OptimizeOperationSize);
 
    /* Run optimization to change x = x + 1 to x++ */
    changes += RunOptimization(toplevels, &OptimizePostInc);
 
    /* Run optimization to change x = x - 1 to x++ */
    changes += RunOptimization(toplevels, &OptimizePostDec);
 
    /* Run optimization for AST_CONV to long for comparison
     * against long AST_LITERAL that is < 65536 */
    changes += RunOptimization(toplevels, &OptimizeLongComparison);

    /* Run optimization for AST_CONV to INT for comparison
     * against long AST_LITERAL that is < 256 */
    changes += RunOptimization(toplevels, &OptimizeIntComparison);

    /* Run optimization for | & ^ || && to put simpler branch on right */
    changes += RunOptimization(toplevels, &OptimizeAssociatveArgs);
 
    /* Run optimization to ensure KIND_BYTE nodes are size 1 */
    changes += RunOptimization(toplevels, &CheckByteNodeSizes);
 
    /* Prune return RETURN nodes */
    changes += RunOptimization(toplevels, &PruneReturnConvNodes);
 
    /* Prune const array access pointer arith */
    changes += RunOptimization(toplevels, &PruneConstArrayAdd);
 
    /* Prune assign followed by return */
    changes += RunOptimization(toplevels, &PruneAssignReturn);
 
    totalChanges += changes;
  } while (changes > 0);

//  printf("Parsed %d AST_CONV nodes\n", gConvCount);
//  printf("Pruned %d AST_* nodes\n", totalChanges);
}
