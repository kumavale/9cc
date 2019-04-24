#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "9cc.h"

// トークンのインデックス
int pos;


// トークナイズした結果のトークン列はこの配列に保存する
// 100個以上のトークンは来ないものとする
Token tokens[100];

Node *code[100];

int is_alnum(char c) {
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') ||
           ( c  == '_');
}

Node *assign() {
    Node *node = add();
    while(consume('='))
        node = new_node('=', node, assign());
    return node;
}

Node *stmt() {
    Node *node;

    if(consume(TK_RETURN)) {
        node = malloc(sizeof(Node));
        node->ty = ND_RETURN;
        node->lhs = (struct Node *)assign();
    } else {
        node = assign();
    }

    if(!consume(';'))
        error("Is not a ';' token: %s", tokens[pos].input);
    return node;
}

void program() {
    int i = 0;
    while(tokens[pos].ty != TK_EOF)
        code[i++] = stmt();
    code[i] = NULL;
}

void expect(int line, int expected, int actual) {
    if(expected == actual)
        return;
    error("%d: %d expected, but got %d\n", line, expected, actual);
    exit(1);
}

void runtest() {
    Vector *vec = new_vector();
    expect(__LINE__, 0, vec->len);

    for(int i=0; i<100; i++)
        vec_push(vec, (void *)(__intptr_t)i);

    expect(__LINE__, 100, vec->len);
    expect(__LINE__,   0, (int)(__intptr_t)vec->data[0]);
    expect(__LINE__,  50, (int)(__intptr_t)vec->data[50]);
    expect(__LINE__,  99, (int)(__intptr_t)vec->data[99]);

    printf("OK\n");
}

Vector *new_vector() {
    Vector *vec = malloc(sizeof(Vector));
    vec->data = malloc(sizeof(void *) * 16);
    vec->capacity = 16;
    vec->len = 0;
    return vec;
}

void vec_push(Vector *vec, void *elem) {
    if(vec->capacity == vec->len) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
    }
    vec->data[vec->len++] = elem;
}

Node *new_node(int ty, Node *lhs, Node *rhs) {
    Node *node = malloc(sizeof(Node));
    node->ty = ty;
    node->lhs = (struct Node *)lhs;
    node->rhs = (struct Node *)rhs;
    return node;
}

Node *new_node_num(int val) {
    //fprintf(stderr, "__LINE__: %d, val: %d\n", __LINE__, val);
    Node *node = malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}

Node *new_node_ident(int val) {
    //fprintf(stderr, "__LINE__: %d, val: %d\n", __LINE__, val);
    Node *node = malloc(sizeof(Node));
    node->ty = ND_IDENT;
    node->name = val;
    return node;
}

int consume(int ty) {
    if(tokens[pos].ty != ty)
        return 0;
    pos++;
    return 1;
}

Node *term() {
    if(tokens[pos].ty == TK_NUM)
        return new_node_num(tokens[pos++].val);

    if(tokens[pos].ty == TK_IDENT)
        return new_node_ident(tokens[pos++].val);

    if(consume('(')) {
        Node *node = add();
        if(!consume(')'))
            error("Open parenthesis has no corresponding closing parenthesis: %s",
                    tokens[pos].input);
        return node;
    }

    error("A token that is neither a number nor an open parenthesis: %s",
            tokens[pos].input);
}

Node *mul() {
    Node *node = term();

    for(;;) {
        if(consume('*'))
            node = new_node('*', node, term());
        else if(consume('/'))
            node = new_node('/', node, term());
        else
            return node;
    }
}

Node *add() {
    Node *node = mul();

    for(;;) {
        if(consume('+'))
            node = new_node('+', node, mul());
        else if(consume('-'))
            node = new_node('-', node, mul());
        else
            return node;
    }
}

void gen_lval(Node *node) {
    if(node->ty != ND_IDENT)
        error("Lvalue of assignment is not a variable");
    //int offset = ('z' - node->name + 1) * 8;
    int offset = (node->name + 1 - 'a') * 8;
    //fprintf(stderr, "[%d]", node->name);
    //fprintf(stderr, "[%d]", offset);
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", offset);
    printf("  push rax\n");
}

void gen(Node *node) {
    if(node->ty == ND_RETURN) {
        gen((Node *)node->lhs);
        printf("  pop rax\n");
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
        return;
    }

    if(node->ty == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }

    if(node->ty == ND_IDENT) {
        gen_lval(node);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    }

    if(node->ty == '=') {
        gen_lval((Node *)node->lhs);
        gen((Node *)node->rhs);

        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        printf("  push rdi\n");
        return;
    }

    gen((Node *)node->lhs);
    gen((Node *)node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch(node->ty) {
        case '+':
            printf("  add rax, rdi\n");
            break;
        case '-':
            printf("  sub rax, rdi\n");
            break;
        case '*':
            printf("  mul rdi\n");
            break;
        case '/':
            printf("  mov rdx, 0\n");
            printf("  div rdi\n");
            break;
    }

    printf("  push rax\n");
}

// エラーを報告するための関数
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// pが指している文字列をトークンに分割してtokensに保存する
void tokenize(char *p) {
    int i = 0;
    while(*p) {
        // 空白文字をスキップ
        if(isspace(*p)) {
            p++;
            continue;
        }

        if(strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
            tokens[i].ty = TK_RETURN;
            tokens[i].input = p;
            i++;
            p += 6;
            continue;
        }

        if('a' <= *p && *p <= 'z') {
            tokens[i].ty = TK_IDENT;
            tokens[i].input = p;
            tokens[i].val = *p;
            i++;
            p++;
            continue;
        }

        if( *p == '+'
         || *p == '-'
         || *p == '*'
         || *p == '/'
         || *p == '('
         || *p == ')'
         || *p == '='
         || *p == ';') {
            tokens[i].ty = *p;
            tokens[i].input = p;
            i++;
            p++;
            continue;
        }

        if(isdigit(*p)) {
            tokens[i].ty = TK_NUM;
            tokens[i].input = p;
            tokens[i].val = strtol(p, &p, 10);
            i++;
            continue;
        }

        error("Failured tokenize: %s\n", p);
        exit(1);
    }

    tokens[i].ty = TK_EOF;
    tokens[i].input = p;
}

int main(int argc, char **argv) {
    if(argc != 2) {
        error("Invalid argument\n");
        return 1;
    }

    // 仮の引数チェック
    // "-test"の時は runtestを呼ぶ
    if(!strcmp(argv[1], "-test")) {
        runtest();
        return 0;
    }

    // トークナイズしてパースする
    tokenize(argv[1]);
    program();

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // プロローグ
    // 変数26個分の領域を確保する
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, 208\n"); // 8 * 26

    // 先頭の式から順にコード生成
    for(int i=0; code[i]; i++) {
        gen(code[i]);

        // 式の評価結果としてスタックに一つの値が残っている
        // はずなので, スタックが溢れないようにポップしておく
        printf("  pop rax\n");
    }

    // エピローグ
    // 最後の式の結果がRAXに残っているのでそれが返り値になる
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");

    return 0;
}
