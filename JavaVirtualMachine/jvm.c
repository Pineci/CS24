#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "jvm.h"
#include "read_class.h"

typedef uint8_t u1;
typedef uint16_t u2;
typedef uint32_t u4;
typedef int8_t s1;
typedef int16_t s2;
typedef int32_t s4;

typedef struct{
    u2 max_length;
    u2 size;
    s4 *stack;
} stack_t;

stack_t *stack_create(u2 max_length){
    stack_t *s = malloc(sizeof(stack_t));
    s->max_length = max_length;
    s->size = 0;
    s->stack = malloc(sizeof(s4)*max_length);
    return s;
}

void stack_push(stack_t *s, s4 val){
    assert(s->size != s->max_length);
    s->stack[s->size] = val;
    s->size++;
}

s4 stack_pop(stack_t *s){
    assert(s->size != 0);
    s4 val = s->stack[s->size-1];
    s->size--;
    return val;
}

void stack_free(stack_t *s){
    free(s->stack);
    free(s);
}

/** The name of the method to invoke to run the class file */
const char *MAIN_METHOD = "main";
/**
 * The "descriptor" string for main(). The descriptor encodes main()'s signature,
 * i.e. main() takes a String[] and returns void.
 * If you're interested, the descriptor string is explained at
 * https://docs.oracle.com/javase/specs/jvms/se12/html/jvms-4.html#jvms-4.3.2.
 */
const char *MAIN_DESCRIPTOR = "([Ljava/lang/String;)V";

s4 bin_op(jvm_instruction_t instruct, s4 val1, s4 val2){
    s4 val = 0;
    switch(instruct){
        case i_iadd:
            val = val1+val2;
            break;
        case i_isub:
            val = val2-val1;
            break;
        case i_imul:
            val = val2*val1;
            break;
        case i_idiv:
            val = val2/val1;
            break;
        case i_irem:
            val = val2 % val1;
            break;
        default:
            break;
    }
    return val;
}

bool check_jump(jvm_instruction_t instruct, s4 a, s4 b){
    bool jump = false;
    if(instruct == i_ifeq || instruct == i_if_icmpeq){
        jump = a == b;
    } else if (instruct == i_ifne || instruct == i_if_icmpne){
        jump = a != b;
    } else if (instruct == i_iflt || instruct == i_if_icmplt){
        jump = a < b;
    } else if (instruct == i_ifge || instruct == i_if_icmpge){
        jump = a >= b;
    } else if (instruct == i_ifgt || instruct == i_if_icmpgt){
        jump = a > b;
    } else if (instruct == i_ifle || instruct == i_if_icmple){
        jump = a <= b;
    } else if (instruct == i_goto){
        jump = true;
    }
    return jump;
}

/**
 * Runs a method's instructions until the method returns.
 *
 * @param method the method to run
 * @param locals the array of local variables, including the method parameters.
 *   Except for parameters, the locals are uninitialized.
 * @param class the class file the method belongs to
 * @return if the method returns an int, a heap-allocated pointer to it;
 *   if the method returns void, NULL
 */
 int32_t *execute(method_t *method, int32_t *locals, class_file_t *class) {
     /* You should remove these casts to void in your solution.
     * They are just here so the code compiles without warnings. */
     code_t code = method->code;
     stack_t *stack = stack_create(code.max_stack);
     u2 pc = 0;
     bool done = false;
     s4 *return_val = NULL;
     while(!done){
         jvm_instruction_t instruct = code.code[pc];

         if (instruct == i_bipush){
             s1 byte = code.code[pc+1];
             stack_push(stack, (s4)byte);
             pc += 2;
         } else if (instruct == i_sipush){
             u1 b1 = code.code[pc+1];
             u1 b2 = code.code[pc+2];
             s2 val = (b1 << 8) | b2;
             stack_push(stack, (s4)val);
             pc += 3;
         } else if (i_iadd <= instruct && instruct <= i_irem){
             s4 val1 = stack_pop(stack);
             s4 val2 = stack_pop(stack);
             stack_push(stack, bin_op(instruct, val1, val2));
             pc++;
         } else if (instruct == i_ineg){
             s4 val = stack_pop(stack);
             stack_push(stack, -1*val);
             pc++;
         } else if (i_iconst_m1 <= instruct && instruct <= i_iconst_5){
             stack_push(stack, instruct-3);
             pc++;
         } else if (instruct == i_return){
             done = true;
         } else if (instruct == i_getstatic){
             pc += 3;
         } else if (instruct == i_invokevirtual){
             s4 val = stack_pop(stack);
             printf("%d\n", val);
             pc += 3;
         } else if (instruct == i_iload){
             u1 addr = code.code[pc+1];
             s4 val = locals[addr];
             stack_push(stack, val);
             pc += 2;
         } else if (instruct == i_istore){
             u1 addr = code.code[pc+1];
             locals[addr] = stack_pop(stack);
             pc += 2;
         } else if (instruct == i_iinc){
             u1 addr = code.code[pc+1];
             s1 val = code.code[pc+2];
             locals[addr] += (s4)val;
             pc += 3;
         } else if (i_iload_0 <= instruct && instruct <= i_iload_3){
             stack_push(stack, locals[instruct - i_iload_0]);
             pc++;
         } else if (i_istore_0 <= instruct && instruct <= i_istore_3){
             locals[instruct - i_istore_0] = stack_pop(stack);
             pc++;
         } else if (instruct == i_ldc){
             u1 b = code.code[pc+1];
             cp_info *cp = get_constant(&(class->constant_pool), b);
             CONSTANT_Integer_info *info = (CONSTANT_Integer_info*)cp->info;
             stack_push(stack, info->bytes);
             pc += 2;
         } else if (i_ifeq <= instruct && instruct <= i_goto){
             s4 a = 0;
             s4 b = 0;
             if (instruct != i_goto){
                 a = stack_pop(stack);
             }
             if (i_if_icmpeq <= instruct && instruct <= i_if_icmple){
                 b = a;
                 a = stack_pop(stack);
             }
             if (check_jump(instruct, a, b)){
                 u1 b1 = code.code[pc+1];
                 u1 b2 = code.code[pc+2];
                 s2 val = (b1 << 8) | b2;
                 pc += val;
             } else {
                 pc += 3;
             }
         } else if (instruct == i_ireturn){
             return_val = malloc(sizeof(s4));
             *return_val = stack_pop(stack);
             done = true;
         } else if (instruct == i_invokestatic){
             u1 b1 = code.code[pc+1];
             u1 b2 = code.code[pc+2];
             u2 addr = (b1 << 8) | b2;

             method_t *new_method = find_method_from_index(addr, class);
             s4 *new_locals = malloc(sizeof(s4)*new_method->code.max_locals);
             u2 n = get_number_of_parameters(new_method);

             for(u2 i = 0; i < n; i++){
                 s4 val = stack_pop(stack);
                 new_locals[n-1-i] = val;
             }

             s4 *ret = execute(new_method, new_locals, class);
             if (ret != NULL){
                 stack_push(stack, *ret);
                 free(ret);
             }
             free(new_locals);
             pc += 3;
         } else {

         }
     }
     stack_free(stack);
     return return_val;
 }

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <class file>\n", argv[0]);
        return 1;
    }

    // Open the class file for reading
    FILE *class_file = fopen(argv[1], "r");
    assert(class_file && "Failed to open file");

    // Parse the class file
    class_file_t class = get_class(class_file);
    int error = fclose(class_file);
    assert(!error && "Failed to close file");

    // Execute the main method
    method_t *main_method = find_method(MAIN_METHOD, MAIN_DESCRIPTOR, &class);
    assert(main_method && "Missing main() method");
    /* In a real JVM, locals[0] would contain a reference to String[] args.
     * But since TeenyJVM doesn't support Objects, we leave it uninitialized. */
    int32_t locals[main_method->code.max_locals];
    int32_t *result = execute(main_method, locals, &class);
    assert(!result && "main() should return void");

    // Free the internal data structures
    free_class(&class);
}
