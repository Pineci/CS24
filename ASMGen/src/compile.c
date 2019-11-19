#include <stdlib.h>
#include <stdio.h>

#include "compile.h"

//Loads the index of the variable into %rcx
void load_var_idx(char c){
    printf("    movq $0x%x, %%rcx\n", c - 'A');
    puts("    imulq $0xffffffffffffffff, %rcx");
}

uint32_t cond_counter = 0;

bool compile_ast(node_t *node) {
    if (node->type == NUM){
        num_node_t *num = (num_node_t*)node;
        printf("    movq $0x%lx, %%rax\n", num->value);
        return true;
    } else if (node->type == PRINT){
        print_node_t *print = (print_node_t*)node;
        compile_ast(print->expr);
        puts("    movq %rax, %rdi\n"
             "    call print_int"
        );
        return true;
    } else if (node->type == BINARY_OP){
        binary_node_t *bin = (binary_node_t*)node;
        compile_ast(bin->left);
        puts("    push %rax");
        compile_ast(bin->right);
        puts("    pop %rcx");
        if (bin->op == '+'){
            puts("    addq %rcx, %rax");
        } else if(bin->op == '-'){
            puts("    subq %rax, %rcx\n"
                 "    movq %rcx, %rax"
            );
        } else if(bin->op == '*'){
            puts("    imulq %rcx, %rax");
        } else if(bin->op == '/'){
            puts("    movq %rax, %rdi\n"
                 "    movq %rcx, %rax\n"
                 "    shrq $0x3f, %rcx\n"
                 "    movq $0x0, %rdx\n"
                 "    subq %rcx, %rdx\n"
                 "    idiv %rdi"
             );
        } else {
            puts("    cmp %rax, %rcx");
            if(bin->op == '<'){
                printf("    setl %%al\n");
            } else if(bin->op == '='){
                printf("    sete %%al\n");
            } else if(bin->op == '>'){
                printf("    setg %%al\n");
            }
        }
        return true;
    } else if (node->type == VAR){
        var_node_t *var = (var_node_t*)node;
        load_var_idx(var->name);
        puts("    movq 0x0(%rbp, %rcx, 8), %rax");
        return true;
    } else if (node->type == LET){
        let_node_t *let = (let_node_t*)node;
        compile_ast(let->value);
        load_var_idx(let->name);
        puts("    movq %rax, 0x0(%rbp, %rcx, 8)");
        return true;
    } else if (node->type == LABEL){
        label_node_t *label = (label_node_t*)node;
        printf( "L%s:\n", label->label);
        return true;
    } else if (node->type == GOTO){
        goto_node_t *to = (goto_node_t*)node;
        printf("    jmp L%s\n", to->label);
        return true;
    } else if (node->type == COND){
        cond_node_t *cond = (cond_node_t*)node;
        uint32_t count = cond_counter;
        compile_ast(cond->condition);
        puts("    test $0xf, %al");
        printf("    je C%d\n", count);
        cond_counter++;
        compile_ast(cond->if_branch);
        printf("C%d:\n", count);
        return true;
    }
    return false; // Something unexpected happened
}
