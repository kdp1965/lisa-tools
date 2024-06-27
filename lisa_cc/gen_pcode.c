// Copyright 2019 Ken Pettit <pettitkd@gmail.com>
// Releaed under the MIT license.

// Optimizations to the Abstract Syntax Tree for the
// Pico16 soft processor.

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "lisacc.h"

/*
======================================================================
Generate p-code for the given node.
======================================================================
*/
static void GenerateNodePcode(Node *v, Node **vsource)
{
  int   i;

  if (!v)
    return;

  /* Iterate to node children */
  switch (v->kind)
  {
    case AST_FUNC:
      GenerateNodePcode(v->body, &v->body);
      break;

    case AST_FUNCALL:
    case AST_FUNCPTR_CALL:
        for (i = 0; i < vec_len(v->args); i++)
          GenerateNodePcode(vec_get(v->args,i), (Node **) &v->args->body[i]);
        break;

    case AST_CONV:
    case AST_INIT:
      GenerateNodePcode(v->operand, &v->operand);
      break;

    case AST_DECL:
      if (v->declinit)
      {
         for (i = 0; i < vec_len(v->declinit); i++)
           GenerateNodePcode(vec_get(v->declinit,i), (Node **) &v->declinit->body[i]);
      }
      break;

    case AST_IF:
    case AST_TERNARY:
      GenerateNodePcode(v->cond, &v->cond);
      GenerateNodePcode(v->then, &v->then);
      GenerateNodePcode(v->els, &v->els);
      break;

    case AST_STRUCT_REF:
      GenerateNodePcode(v->struc, &v->struc);
      break;

    case AST_RETURN:
      GenerateNodePcode(v->retval, &v->retval);
      break;

    case AST_COMPOUND_STMT:
      for (i = 0; i < vec_len(v->stmts); i++)
        GenerateNodePcode(vec_get(v->stmts,i), (Node**) &v->stmts->body[i]);
      break;

    case AST_ADDR:
    case AST_DEREF:
    case OP_PRE_INC:
    case OP_PRE_DEC:
    case OP_POST_INC:
    case OP_POST_DEC:
    case OP_CAST:
    case '!':
      GenerateNodePcode(v->operand, &v->operand);
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
    case '=':
    case '^':
    case '%':
    case '<':
    case '>':
    case '-':
    case '+':
      GenerateNodePcode(v->left, &v->left);
      GenerateNodePcode(v->right, &v->right);
      break;
  }
}

/*
======================================================================
Run all known optimizations passes on the AST top level
======================================================================
*/
void GeneratePcode(Vector *toplevels)
{
  int i;
  Node *v;

  /* Loop across all top level AST tree nodes */
  for (i = 0; i < vec_len(toplevels); i++)
  {
    v = vec_get(toplevels, i);
    GenerateNodePcode(v, NULL);
  }
}

