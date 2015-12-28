#ifndef FUNCTION_H_
# define FUNCTION_H_

# include "object.h"
# include "ast.h"
# include "eval.h"

extern wsky_Class wsky_Function_CLASS;



typedef struct wsky_Function_s wsky_Function;

struct wsky_Function_s {
  wsky_OBJECT_HEAD

  /* NULL if anonymous */
  char *name;

  wsky_Scope *globalScope;

  wsky_FunctionNode *node;
};


/**
 * @param name The name or NULL if the function is anonymous
 */
wsky_Function *wsky_Function_new(const char *name,
                                 const wsky_FunctionNode *node,
                                 wsky_Scope *globalScope);

wsky_ReturnValue wsky_Function_call(wsky_Object *object,
                                    unsigned parameterCount,
                                    wsky_Value *parameters);

bool wsky_isFunction(const wsky_Value value);


#endif /* !FUNCTION_H_ */