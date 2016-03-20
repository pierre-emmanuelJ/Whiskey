#include <assert.h>
#include <string.h>
#include "whiskey.h"


typedef wsky_Operator Operator;
typedef wsky_Position Position;
typedef wsky_Token Token;
typedef wsky_TokenList TokenList;
typedef wsky_SyntaxError SyntaxError;
typedef wsky_ASTNode Node;
typedef wsky_ASTNodeList NodeList;
typedef wsky_ParserResult ParserResult;


#define OP(name) (wsky_Operator_ ## name)

static inline bool isOpToken(Token *token) {
  return token->type == wsky_TokenType_OPERATOR;
}



static inline ParserResult createResultFromError(SyntaxError e) {
  ParserResult r = {
    .success = 0,
    .node = NULL,
    .syntaxError = e,
  };
  return r;
}

static inline ParserResult createNodeResult(Node *n) {
  ParserResult r = {
    .success = 1,
    .node = n,
  };
  return r;
}

static inline ParserResult createError(const char *msg, Position pos) {
  assert(!wsky_Position_isUnknown(&pos));
  SyntaxError e = wsky_SyntaxError_create(msg, pos);
  return createResultFromError(e);
}

/**
 * Prefer createError(). Use this one only if there is no position.
 */
static inline ParserResult createEofError(const char *msg) {
  /*
   * We have no position, so let's return an invalid one.
   * It will be replaced by a valid one later.
   */
  SyntaxError e = wsky_SyntaxError_create(msg, wsky_Position_UNKNOWN);
  return createResultFromError(e);
}

/**
 * Prefer createError(). Use this one only if there is no position.
 */
static inline ParserResult createUnexpectedEofError() {
  return createEofError("Unexpected end of file");
}


static inline ParserResult createUnexpectedTokenError(const Token *t) {
  char *message = wsky_asprintf("Unexpected '%s'", t->string);
  ParserResult r = createError(message, t->begin);
  wsky_free(message);
  return r;
}

static const ParserResult ParserResult_NULL = {
  .success = 1,
  .node = NULL,
};



static ParserResult parseExpr(TokenList **listPointer);



/* Returns a literal (string, int or float) or NULL */
static ParserResult parseLiteral(TokenList **listPointer) {
  if (!*listPointer) {
    return ParserResult_NULL;
  }

  Token *token = &(*listPointer)->token;
  if (!wsky_Token_isLiteral(token)) {
    return ParserResult_NULL;
  }
  Node *node = (Node *) wsky_LiteralNode_new(token);
  *listPointer = (*listPointer)->next;
  return createNodeResult(node);
}

/* Returns an HTML node or NULL */
static ParserResult parseHtml(TokenList **listPointer) {
  if (!*listPointer) {
    return ParserResult_NULL;
  }

  Token *token = &(*listPointer)->token;
  if (token->type != wsky_TokenType_HTML) {
    return ParserResult_NULL;
  }
  Node *node = (Node *) wsky_HtmlNode_new(token);
  *listPointer = (*listPointer)->next;
  return createNodeResult(node);
}


static Token *tryToReadIdentifier(TokenList **listPointer) {
  if (!*listPointer)
    return NULL;

  Token *token = &(*listPointer)->token;
  if (token->type != wsky_TokenType_IDENTIFIER)
    return NULL;

  *listPointer = (*listPointer)->next;

  return token;
}

/* Returns a string or NULL */
static const char *parseIdentifierString(TokenList **listPointer) {
  Token *token = tryToReadIdentifier(listPointer);
  if (!token)
    return NULL;
  return token->string;
}

/* Returns an identifier or NULL */
static wsky_IdentifierNode *parseIdentifierNode(TokenList **listPointer) {
  Token *token = tryToReadIdentifier(listPointer);
  if (!token)
    return NULL;

  wsky_IdentifierNode *node = wsky_IdentifierNode_newFromToken(token);
  return node;
}


/* Returns a Token or NULL */
static Token *tryToReadOperator(TokenList **listPointer,
                                Operator expectedOp) {
  if (!*listPointer)
    return NULL;

  Token *token = &(*listPointer)->token;
  if (!isOpToken(token))
    return NULL;

  Operator op = token->v.operator;
  if (op != expectedOp)
    return NULL;

  *listPointer = (*listPointer)->next;
  return token;
}

/* Returns a Token or NULL */
static Token *tryToReadKeyword(TokenList **listPointer,
                               wsky_Keyword expectedKeyword) {
  if (!*listPointer) {
    return NULL;
  }
  Token *token = &(*listPointer)->token;
  if (token->type != wsky_TokenType_KEYWORD) {
    return NULL;
  }
  wsky_Keyword keyword = token->v.keyword;
  if (keyword != expectedKeyword) {
    return NULL;
  }
  *listPointer = (*listPointer)->next;
  return token;
}



static Node *createSelfNode(Position position) {
  return (Node *)wsky_IdentifierNode_new(NULL,
                                         wsky_ASTNodeType_SELF,
                                         position);
}

static Node *createSuperNode(Position position) {
  return (Node *)wsky_IdentifierNode_new(NULL,
                                         wsky_ASTNodeType_SUPER,
                                         position);
}

static Node *createSuperclassNode(Position position) {
  return (Node *)wsky_IdentifierNode_new(NULL,
                                         wsky_ASTNodeType_SUPERCLASS,
                                         position);
}

/* Returns an identifier or NULL */
static ParserResult parseIdentifier(TokenList **listPointer) {
  Token *token;
  token = tryToReadOperator(listPointer, OP(AT));
  if (token)
    return createNodeResult(createSelfNode(token->begin));

  token = tryToReadKeyword(listPointer, wsky_Keyword_SUPERCLASS);
  if (token)
    return createNodeResult(createSuperclassNode(token->begin));

  token = tryToReadKeyword(listPointer, wsky_Keyword_SUPER);
  if (token)
    return createNodeResult(createSuperNode(token->begin));

  Node *node = (Node *)parseIdentifierNode(listPointer);
  if (!node)
    return ParserResult_NULL;

  return createNodeResult(node);
}


static ParserResult parseSequenceImpl(TokenList **listPointer,
                                      Operator separatorOperator,
                                      Token *beginToken,
                                      Operator endOperator,
                                      const char *expectedSeparatorErr,
                                      const char *expectedEndErr) {
  NodeList *nodes = NULL;
  bool separated = true;

  while (*listPointer) {
    Token *right = tryToReadOperator(listPointer, endOperator);
    if (right) {
      Node *node = (Node *) wsky_SequenceNode_new(&beginToken->begin,
                                                  nodes);
      return createNodeResult(node);
    }

    if (!separated) {
      Node *lastNode = wsky_ASTNodeList_getLastNode(nodes);
      Position p = lastNode->position;
      wsky_ASTNodeList_delete(nodes);
      return createError(expectedSeparatorErr, p);
    }

    ParserResult r = parseExpr(listPointer);
    if (!r.success) {
      wsky_ASTNodeList_delete(nodes);
      return r;
    }
    wsky_ASTNodeList_addNode(&nodes, r.node);
    separated = tryToReadOperator(listPointer, separatorOperator);
  }
  wsky_ASTNodeList_delete(nodes);
  return createError(expectedEndErr, beginToken->begin);
}


static ParserResult parseSequence(TokenList **listPointer) {
  Token *left = tryToReadOperator(listPointer, OP(LEFT_PAREN));
  if (!left) {
    return ParserResult_NULL;
  }

  return parseSequenceImpl(listPointer,
                           OP(SEMICOLON),
                           left,
                           OP(RIGHT_PAREN),
                           "Expected ';' or ')'", "Expected ')'");
}


static ParserResult checkParams(const NodeList *params) {
  const NodeList *param = params;
  while (param) {
    Node *n = param->node;
    if (n->type != wsky_ASTNodeType_IDENTIFIER)
      return createError("Invalid function parameter", n->position);
    param = param->next;
  }
  return ParserResult_NULL;
}

static ParserResult parseFunction(TokenList **listPointer) {
  Token *left = tryToReadOperator(listPointer, OP(LEFT_BRACE));
  if (!left) {
    return ParserResult_NULL;
  }

  TokenList *begin = *listPointer;
  ParserResult paramPr;
  paramPr = parseSequenceImpl(listPointer,
                              OP(COMMA),
                              left,
                              OP(COLON),
                              "Expected ',' or ':'", "Expected ':'");
  NodeList *params = NULL;
  if (paramPr.success) {
    wsky_SequenceNode *sequence = (wsky_SequenceNode *) paramPr.node;
    params = sequence->children;
    wsky_free(sequence);
  } else {
    *listPointer = begin;
    wsky_SyntaxError_free(&paramPr.syntaxError);
  }

  ParserResult pr = checkParams(params);
  if (!pr.success) {
    wsky_ASTNodeList_delete(params);
    return pr;
  }

  pr = parseSequenceImpl(listPointer,
                         OP(SEMICOLON),
                         left,
                         OP(RIGHT_BRACE),
                         "Expected ';' or '}'", "Expected '}'");
  if (!pr.success) {
    wsky_ASTNodeList_delete(params);
    return pr;
  }
  wsky_SequenceNode *sequence = (wsky_SequenceNode *) pr.node;
  wsky_FunctionNode *func = wsky_FunctionNode_new(left,
                                                  params,
                                                  sequence->children);
  wsky_free(sequence);
  return createNodeResult((Node *) func);
}


// TODO: Rename
static ParserResult parseTerm(TokenList **listPointer) {
  ParserResult result;

  result = parseIdentifier(listPointer);
  if (result.node)
    return result;

  result = parseLiteral(listPointer);
  if (result.node)
    return result;

  result = parseHtml(listPointer);
  if (result.node)
    return result;

  /*
    result = parseTemplatePrint(listPointer);
    if (result.node)
    return result;
  */

  result = parseSequence(listPointer);
  if (!result.success || result.node)
    return result;

  result = parseFunction(listPointer);
  if (!result.success || result.node)
    return result;

  return createUnexpectedTokenError(&(*listPointer)->token);
}


/* Returns a heap allocated string or NULL */
static char *parseMemberName(TokenList **listPointer) {
  Token *token = tryToReadKeyword(listPointer, wsky_Keyword_CLASS);
  if (token)
    return wsky_strdup("class");

  const char *name = parseIdentifierString(listPointer);
  if (!name)
    return NULL;
  return wsky_strdup(name);
}


static ParserResult parseMemberAccess(TokenList **listPointer,
                                      Node *left,
                                      Token *dotToken) {
  char *name = parseMemberName(listPointer);
  if (!name)
    return createError("Expected member name after '.'", dotToken->end);

  wsky_MemberAccessNode *node;
  node = wsky_MemberAccessNode_new(dotToken, left, name);
  wsky_free(name);
  return createNodeResult((Node *) node);
}


static ParserResult parseCall(TokenList **listPointer,
                              Node *left,
                              Token *leftParen) {

  ParserResult pr;
  pr = parseSequenceImpl(listPointer,
                         OP(COMMA),
                         leftParen,
                         OP(RIGHT_PAREN),
                         "Expected ',' or ')'", "Expected ')'");
  if (!pr.success)
    return pr;
  wsky_SequenceNode *sequence = (wsky_SequenceNode *) pr.node;
  wsky_CallNode *callNode = wsky_CallNode_new(leftParen,
                                              left, sequence->children);
  wsky_free(sequence);
  return createNodeResult((Node *) callNode);
}


static ParserResult parseCallDotIndex(TokenList **listPointer) {
  ParserResult leftResult = parseTerm(listPointer);
  if (!leftResult.success)
    return leftResult;

  Node *left = leftResult.node;

  while (*listPointer) {
    Token *token = &(*listPointer)->token;
    if (!isOpToken(token)) {
      break;
    }

    if (token->v.operator == OP(LEFT_PAREN)) {
      *listPointer = (*listPointer)->next;
      ParserResult pr = parseCall(listPointer, left, token);
      if (!pr.success) {
        wsky_ASTNode_delete(left);
        return pr;
      }
      left = pr.node;

    } else if (token->v.operator == OP(DOT)) {
      *listPointer = (*listPointer)->next;
      ParserResult pr = parseMemberAccess(listPointer, left, token);
      if (!pr.success) {
        wsky_ASTNode_delete(left);
        return pr;
      }
      left = pr.node;

    } else {
      break;
    }
  }

  return createNodeResult(left);
}



static ParserResult parseSelf(TokenList **listPointer) {
  if (!*listPointer)
    return createUnexpectedEofError();

  TokenList *begin = *listPointer;
  Token *token = tryToReadOperator(listPointer, OP(AT));
  if (!token) {
    return parseCallDotIndex(listPointer);
  }

  char *right = parseMemberName(listPointer);
  if (!right) {
    *listPointer = begin;
    return parseCallDotIndex(listPointer);
  }
  Node *selfNode = createSelfNode(token->begin);
  Node *dot = (Node *)wsky_MemberAccessNode_new(token, selfNode, right);
  wsky_free(right);
  return createNodeResult(dot);
}


static bool isUnaryOperator(Operator op) {
  return op == OP(MINUS) || op == OP(PLUS) || op == OP(NOT);
}

static ParserResult parseUnary(TokenList **listPointer) {
  if (!*listPointer)
    return createUnexpectedEofError();

  Token *token = &(*listPointer)->token;
  if (!isOpToken(token)) {
    return parseSelf(listPointer);
  }

  Operator op = token->v.operator;
  if (!isUnaryOperator(op))
    return parseSelf(listPointer);
  *listPointer = (*listPointer)->next;
  ParserResult rr = parseUnary(listPointer);
  if (!rr.success) {
    return rr;
  }
  Node *node;
  node = (Node *) wsky_OperatorNode_newUnary(token, op, rr.node);
  return createNodeResult(node);
}


static ParserResult parseFactor(TokenList **listPointer) {
  return parseUnary(listPointer);
}


static ParserResult parseMul(TokenList **listPointer) {
  ParserResult lr = parseFactor(listPointer);
  if (!lr.success)
    return lr;

  Node *left = lr.node;

  while (*listPointer) {
    Token *opToken = &(*listPointer)->token;
    if (!isOpToken(opToken)) {
      break;
    }

    Operator op = opToken->v.operator;
    if (op != OP(STAR) && op != OP(SLASH)) {
      break;
    }

    *listPointer = (*listPointer)->next;
    ParserResult rr = parseFactor(listPointer);
    if (!rr.success) {
      wsky_ASTNode_delete(left);
      return rr;
    }
    left = (Node *) wsky_OperatorNode_new(opToken, left, op, rr.node);
  }

  return createNodeResult(left);
}


static ParserResult parseAdd(TokenList **listPointer) {
  ParserResult lr = parseMul(listPointer);
  if (!lr.success) {
    return lr;
  }
  Node *left = lr.node;

  while (*listPointer) {
    Token *opToken = &(*listPointer)->token;
    if (!isOpToken(opToken)) {
      break;
    }

    Operator op = opToken->v.operator;
    if (op != OP(PLUS) && op != OP(MINUS)) {
      break;
    }

    *listPointer = (*listPointer)->next;
    ParserResult rr = parseMul(listPointer);
    if (!rr.success) {
      wsky_ASTNode_delete(left);
      return rr;
    }
    left = (Node *) wsky_OperatorNode_new(opToken, left, op, rr.node);
  }

  return createNodeResult(left);
}


static bool isComparisonOp(Operator op) {
  return (op == OP(LT) || op == OP(LT_EQ) ||
          op == OP(GT) || op == OP(GT_EQ));
}

static ParserResult parseComparison(TokenList **listPointer) {
  ParserResult lr = parseAdd(listPointer);
  if (!lr.success) {
    return lr;
  }
  Node *left = lr.node;

  while (*listPointer) {
    Token *opToken = &(*listPointer)->token;
    if (!isOpToken(opToken)) {
      break;
    }

    Operator op = opToken->v.operator;
    if (!isComparisonOp(op)) {
      break;
    }

    *listPointer = (*listPointer)->next;
    ParserResult rr = parseAdd(listPointer);
    if (!rr.success) {
      wsky_ASTNode_delete(left);
      return rr;
    }
    left = (Node *) wsky_OperatorNode_new(opToken, left, op, rr.node);
  }

  return createNodeResult(left);
}


static ParserResult parseEquals(TokenList **listPointer) {
  ParserResult lr = parseComparison(listPointer);
  if (!lr.success) {
    return lr;
  }
  Node *left = lr.node;

  while (*listPointer) {
    Token *opToken = &(*listPointer)->token;
    if (!isOpToken(opToken)) {
      break;
    }

    wsky_Operator op = opToken->v.operator;
    if (op != OP(EQUALS) && op != OP(NOT_EQUALS)) {
      break;
    }

    *listPointer = (*listPointer)->next;
    ParserResult rr = parseComparison(listPointer);
    if (!rr.success) {
      wsky_ASTNode_delete(left);
      return rr;
    }
    left = (Node *) wsky_OperatorNode_new(opToken, left, op, rr.node);
  }

  return createNodeResult(left);
}


static ParserResult parseBoolOp(TokenList **listPointer) {
  ParserResult lr = parseEquals(listPointer);
  if (!lr.success) {
    return lr;
  }
  Node *left = lr.node;

  while (*listPointer) {
    Token *opToken = &(*listPointer)->token;
    if (!isOpToken(opToken)) {
      break;
    }

    wsky_Operator op = opToken->v.operator;
    if (op != OP(AND) && op != OP(OR)) {
      break;
    }

    *listPointer = (*listPointer)->next;
    ParserResult rr = parseEquals(listPointer);
    if (!rr.success) {
      wsky_ASTNode_delete(left);
      return rr;
    }
    left = (Node *) wsky_OperatorNode_new(opToken, left, op, rr.node);
  }

  return createNodeResult(left);
}


#include "parser_class.c"


static unsigned parseImportDots(TokenList **listPointer) {
  unsigned count = 0;
  while (tryToReadOperator(listPointer, OP(DOT)))
    count++;
  return count;
}

static ParserResult parseImport(TokenList **listPointer) {
  Token *importToken = tryToReadKeyword(listPointer, wsky_Keyword_IMPORT);
  if (!importToken)
    return ParserResult_NULL;

  unsigned level = parseImportDots(listPointer);

  const char *name = parseIdentifierString(listPointer);
  if (!name)
    return createError("Expected module name", importToken->end);

  wsky_ImportNode *node = wsky_ImportNode_new(importToken->begin,
                                              level, name);
  return createNodeResult((Node *) node);
}



static ParserResult parseIfTest(TokenList **listPointer,
                                Token **ifTokenPointer) {
  Token *ifToken = tryToReadKeyword(listPointer, wsky_Keyword_IF);
  if (!ifToken)
    return ParserResult_NULL;

  if (!*listPointer)
    return createError("Expected condition", ifToken->end);

  ParserResult pr = parseExpr(listPointer);
  if (!pr.success)
    return pr;
  assert(pr.node);

  if (!tryToReadOperator(listPointer, OP(COLON))) {
    wsky_ASTNode_delete(pr.node);
    return createError("Expected colon", ifToken->end);
  }

  if (!*listPointer) {
    wsky_ASTNode_delete(pr.node);
    return createError("Expected expression", ifToken->end);
  }

  if (ifTokenPointer)
    *ifTokenPointer = ifToken;

  return pr;
}

static ParserResult parseIfTestExpr(TokenList **listPointer,
                                    Token **ifTokenPointer,
                                    Node **testPointer,
                                    Node **expressionPointer) {
  *testPointer = *expressionPointer = NULL;

  ParserResult pr = parseIfTest(listPointer, ifTokenPointer);
  if (!pr.success || !pr.node)
    return pr;

  *testPointer = pr.node;

  pr = parseExpr(listPointer);
  if (!pr.success)
    return pr;
  *expressionPointer = pr.node;

  return ParserResult_NULL;
}


static ParserResult parseIf(TokenList **listPointer) {
  Token *ifToken;
  Node *test = NULL;
  Node *expression = NULL;
  ParserResult pr = parseIfTestExpr(listPointer, &ifToken,
                                    &test, &expression);
  if (!pr.success)
    return pr;
  if (!test)
    return ParserResult_NULL;

  NodeList *tests = NULL;
  NodeList *expressions = NULL;

  wsky_ASTNodeList_addNode(&tests, test);
  wsky_ASTNodeList_addNode(&expressions, expression);

  Node *elseNode = NULL;

  while (*listPointer) {
    Token *elseToken = tryToReadKeyword(listPointer, wsky_Keyword_ELSE);
    if (!elseToken)
      break;
    if (!*listPointer) {
      wsky_ASTNodeList_delete(tests);
      wsky_ASTNodeList_delete(expressions);
      return createError("Expected colon or 'if'", elseToken->end);
    }

    test = NULL;
    expression = NULL;
    pr = parseIfTestExpr(listPointer, NULL, &test, &expression);
    if (!pr.success) {
      wsky_ASTNodeList_delete(tests);
      wsky_ASTNodeList_delete(expressions);
      return pr;
    }

    if (test) {
      assert(expression);
      wsky_ASTNodeList_addNode(&tests, test);
      wsky_ASTNodeList_addNode(&expressions, expression);
    } else {
      if (!tryToReadOperator(listPointer, OP(COLON))) {
        wsky_ASTNodeList_delete(tests);
        wsky_ASTNodeList_delete(expressions);
        return createError("Expected colon or 'if' after 'else'",
                           elseToken->end);
      }

      pr = parseExpr(listPointer);
      if (!pr.success) {
        wsky_ASTNodeList_delete(tests);
        wsky_ASTNodeList_delete(expressions);
        return pr;
      }

      elseNode = pr.node;
      break;
    }
  }

  Node *node = (Node *)wsky_IfNode_new(ifToken->begin,
                                       tests, expressions,
                                       elseNode);
  return createNodeResult(node);
}


static ParserResult parseVar(TokenList **listPointer) {
  Token *varToken = tryToReadKeyword(listPointer, wsky_Keyword_VAR);
  if (!varToken)
    return ParserResult_NULL;

  const char *name = parseIdentifierString(listPointer);
  if (!name)
    return createError("Expected variable name", varToken->end);

  Node *rightNode = NULL;
  if (tryToReadOperator(listPointer, OP(ASSIGN))) {
    if (!*listPointer)
      return createUnexpectedEofError();

    ParserResult pr = parseExpr(listPointer);
    if (!pr.success)
      return pr;
    rightNode = pr.node;
  }
  wsky_VarNode *node = wsky_VarNode_new(varToken, name, rightNode);
  return createNodeResult((Node *) node);
}


static ParserResult parseExportAssign(TokenList **listPointer,
                                      const Token *exportToken) {
  const char *name = parseIdentifierString(listPointer);
  if (!name)
    return createError("Expected variable name after `export`",
                       exportToken->end);

  Node *rightNode = NULL;

  if (tryToReadOperator(listPointer, OP(ASSIGN))) {
    if (!*listPointer)
      return createUnexpectedEofError();

    ParserResult pr = parseExpr(listPointer);
    if (!pr.success)
      return pr;
    rightNode = pr.node;
  }

  wsky_ExportNode *node = wsky_ExportNode_new(exportToken->begin,
                                              name, rightNode);
  return createNodeResult((Node *) node);
}

static ParserResult parseExport(TokenList **listPointer) {
  Token *exportToken = tryToReadKeyword(listPointer, wsky_Keyword_EXPORT);
  if (!exportToken)
    return ParserResult_NULL;

  ParserResult pr = parseClass(listPointer);
  if (!pr.success)
    return pr;
  if (pr.node) {
    wsky_ClassNode *class = (wsky_ClassNode *)pr.node;
    wsky_ExportNode *node = wsky_ExportNode_new(exportToken->begin,
                                                class->name, pr.node);
    return createNodeResult((Node *) node);
  }

  return parseExportAssign(listPointer, exportToken);
}


static Node *parseLValue(TokenList **listPointer) {
  TokenList *begin = *listPointer;
  ParserResult pr = parseSelf(listPointer);
  if (!pr.success) {
    wsky_SyntaxError_free(&pr.syntaxError);
    *listPointer = begin;
    return NULL;
  }
  return pr.node;
}


static ParserResult parseAssignement(TokenList **listPointer) {
  ParserResult pr;
  TokenList *begin = *listPointer;

  Node *leftNode = parseLValue(listPointer);
  if (!leftNode)
    return ParserResult_NULL;

  Token *eqToken = tryToReadOperator(listPointer, OP(ASSIGN));
  if (!eqToken) {
    wsky_ASTNode_delete(leftNode);
    *listPointer = begin;
    return ParserResult_NULL;
  }

  pr = parseExpr(listPointer);
  if (!pr.success) {
    wsky_ASTNode_delete(leftNode);
    return pr;
  }
  Node *rightNode = pr.node;

  wsky_AssignmentNode *node;
  node = wsky_AssignmentNode_new(eqToken, leftNode, rightNode);
  return createNodeResult((Node *) node);
}

static ParserResult parseCoumpoundExpr(TokenList **listPointer) {
  ParserResult pr;

  pr = parseClass(listPointer);
  if (!pr.success || pr.node)
    return pr;

  pr = parseVar(listPointer);
  if (!pr.success || pr.node)
    return pr;

  pr = parseImport(listPointer);
  if (!pr.success || pr.node)
    return pr;

  pr = parseIf(listPointer);
  if (!pr.success || pr.node)
    return pr;

  pr = parseExport(listPointer);
  if (!pr.success || pr.node)
    return pr;

  pr = parseAssignement(listPointer);
  if (!pr.success || pr.node)
    return pr;

  return parseBoolOp(listPointer);
}

static ParserResult parseExpr(TokenList **listPointer) {
  if (!*listPointer)
    return createUnexpectedEofError();

  return parseCoumpoundExpr(listPointer);
}


/*
 * 'Unexpected EOF' errors have an invalid position (-1) at the time
 * of their creation, because the line, the index and the column
 * numbers are unknown. This function sets their position to a real
 * value.
 */
static void setEOFErrorPosition(ParserResult *result,
                                TokenList *begin) {
  if (!result->success) {
    wsky_SyntaxError *e = &result->syntaxError;
    if (e->position.index == -1 && begin) {
      /* That's an "unexpected EOF error" */
      Token *lastToken = &wsky_TokenList_getLast(begin)->token;
      e->position = lastToken->end;
    }
  }
}

static ParserResult parseProgram(TokenList **listPointer) {
  NodeList *nodes = NULL;

  assert(*listPointer);
  Token *firstToken = &(*listPointer)->token;
  while (*listPointer) {
    ParserResult pr = parseExpr(listPointer);
    if (!pr.success) {
      wsky_ASTNodeList_delete(nodes);
      return pr;
    }
    wsky_ASTNodeList_addNode(&nodes, pr.node);

    if (!*listPointer)
      break;
    Token *semi = tryToReadOperator(listPointer, OP(SEMICOLON));
    if (!semi) {
      wsky_ASTNodeList_delete(nodes);
      return createUnexpectedTokenError(&(*listPointer)->token);
    }
  }

  Position begin = firstToken->begin;
  wsky_SequenceNode *seqNode = wsky_SequenceNode_new(&begin, nodes);
  seqNode->program = true;
  return createNodeResult((Node *) seqNode);
}


ParserResult wsky_parse(TokenList *tokens) {
  if (!tokens) {
    wsky_SequenceNode *node;
    node = wsky_SequenceNode_new(&wsky_Position_UNKNOWN, NULL);
    node->program = true;
    return createNodeResult((Node *) node);
  }

  TokenList *begin = tokens;

  ParserResult r = parseProgram(&tokens);
  setEOFErrorPosition(&r, begin);
  return r;
}


ParserResult wsky_parseLine(TokenList *tokens) {
  return wsky_parse(tokens);
}


wsky_ParserResult wsky_parseTemplate(wsky_TokenList *tokens) {
  return wsky_parse(tokens);
}

static ParserResult parseFromLexerResult(wsky_LexerResult lr) {
  if (!lr.success)
    return createResultFromError(lr.syntaxError);

  wsky_TokenList_deleteComments(&lr.tokens);

  ParserResult pr = wsky_parse(lr.tokens);
  wsky_TokenList_delete(lr.tokens);
  if (!pr.success)
    return pr;

  return pr;
}

ParserResult wsky_parseString(const char *string) {
  return parseFromLexerResult(wsky_lexFromString(string));
}

wsky_ParserResult wsky_parseFile(wsky_ProgramFile *file) {
  return parseFromLexerResult(wsky_lexFromFile(file));
}

wsky_ParserResult wsky_parseTemplateString(const char *string) {
  wsky_LexerResult lr = wsky_lexTemplateFromString(string);
  if (!lr.success)
    return createResultFromError(lr.syntaxError);

  ParserResult pr = wsky_parseTemplate(lr.tokens);
  wsky_TokenList_delete(lr.tokens);
  if (!pr.success)
    return pr;

  return pr;
}
