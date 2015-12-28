/* Included in eval.c */


static ReturnValue boolAnd(bool left, Value right) {
  if (IS_BOOL(right)) {
    wsky_RETURN_BOOL(left && right.v.intValue);
  }
  return wsky_ReturnValue_FALSE;
}

static ReturnValue boolOr(bool left, Value right) {
  if (IS_BOOL(right)) {
    wsky_RETURN_BOOL(left || right.v.boolValue);
  }
  return wsky_ReturnValue_TRUE;
}


static ReturnValue boolEquals(bool left, Value right) {
  if (IS_BOOL(right)) {
    wsky_RETURN_BOOL(left == right.v.intValue);
  }
  return wsky_ReturnValue_FALSE;
}

static ReturnValue boolNotEquals(bool left, Value right) {
  if (IS_BOOL(right)) {
    wsky_RETURN_BOOL(left != right.v.boolValue);
  }
  return wsky_ReturnValue_TRUE;
}


static ReturnValue evalBinOperatorBool(bool left,
                                       wsky_Operator operator,
                                       Value right) {

  switch (operator) {
  case wsky_Operator_EQUALS: return boolEquals(left, right);
  case wsky_Operator_NOT_EQUALS: return boolNotEquals(left, right);

  case wsky_Operator_AND: return boolAnd(left, right);
  case wsky_Operator_OR: return boolOr(left, right);

  default:
    break;
  }

  return createUnsupportedBinOpError("Bool",
                                     wsky_Operator_toString(operator),
                                     right);
}

static ReturnValue evalUnaryOperatorBool(wsky_Operator operator,
                                         bool right) {
  switch (operator) {
  case wsky_Operator_NOT: wsky_RETURN_BOOL(!right);

  default:
    break;
  }

  return createUnsupportedUnaryOpError(wsky_Operator_toString(operator),
                                       "Bool");
}