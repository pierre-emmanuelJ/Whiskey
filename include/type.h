#ifndef TYPE_H_
# define TYPE_H_

/**
 * @addtogroup Value
 * @{
 */

/**
 * The type of a wsky_Value.
 */
typedef enum {
  wsky_Type_BOOL,
  wsky_Type_INT,
  wsky_Type_FLOAT,
  wsky_Type_OBJECT
} wsky_Type;

/**
 * @}
 */

#endif /* !TYPE_H_ */
