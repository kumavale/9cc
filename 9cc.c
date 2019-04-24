#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"

int pos;


// トークナイズした結果のトークン列はこの配列に保存する
// 100個以上のトークンは来ないものとする
Token tokens[100];

void expect(int line, int expected, int actual) {
    if(expected == actual)
        return;
    fprintf(stderr, "%d: %d expected, but got %d\n",
            line, expected, actual);
    exit(1);
}

void runtest() {
    Vector *vec = new_vector();
    expect(__LINE__, 0, vec->len);

    for(int i=0; i<100; i++)
        vec_push(vec, (void *)i);

    expect(__LINE__, 100, vec->len);
    expect(__LINE__,   0, (int)vec->data[0]);
    expect(__LINE__,  50, (int)vec->data[50]);
    expect(__LINE__,  99, (int)vec->data[99]);

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
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}

int consume(int ty) {
    if(tokens[pos].ty != ty)
        return 0;
    pos++;
    return 1;
}

Node *term() {
    if(consume('(')) {
        Node *node = add();
        if(!consume(')'))
            fprintf(stderr,
                    "Open parenthesis has no corresponding closing parenthesis: %s",
                    tokens[pos].input);
        return node;
    }

    if(tokens[pos].ty == TK_NUM)
        return new_node_num(tokens[pos++].val);

    fprintf(stderr, "A token that is neither a number nor an open parenthesis: %s",
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

void gen(Node *node) {
    if(node->ty == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

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
    }

    printf("  push rax\n");
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

        if( *p == '+'
         || *p == '-'
         || *p == '*'
         || *p == '/'
         || *p == '('
         || *p == ')') {
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

        fprintf(stderr, "Failured tokenize: %s\n", p);
        exit(1);
    }

    tokens[i].ty = TK_EOF;
    tokens[i].input = p;
}

// エラーを報告するための関数
void error(int i) {
    fprintf(stderr, "Unexpected token: %s\n", tokens[i].input);
    exit(1);
}

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "Invalid argument\n");
        return 1;
    }

    // 仮の引数チェック
    // "-test"の時は runtestを呼ぶ
    if(!strcmp(argv[1], "-test")) {
        runtest();
        exit(0);
    }

    // トークナイズしてパースする
    tokenize(argv[1]);
    Node *node = add();

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // 抽象構文木を下りながらコード生成
    gen(node);

    // スタックトップに式全体の値が残っているはずなので
    // それをRAXにロードして関数からの返り値とする
    printf("  pop rax\n");
    printf("  ret\n");

    return 0;
}
