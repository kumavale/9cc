#include "9cc.h"

// All local variable instances created during parsing are
// accumulated to this list.
LVar *locals;

static Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs  = lhs;
    node->rhs  = rhs;
    return node;
}

static Node *new_num(long val) {
    Node *node = new_node(ND_NUM);
    node->val  = val;
    return node;
}

// Find a local variable by name.
static LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next) {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
            return var;
        }
    }
    return NULL;
}

static Node *stmt(void);
static Node *expr(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);

Node *code[100];

// program = stmt*
Node *program(void) {
    int i = 0;
    locals = NULL;
    while (!at_eof()) {
        code[i++] = stmt();
    }
    code[i] = NULL;
}

// stmt = return expr ";"
//      | "{" stmt* "}"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | expr ";"
static Node *stmt(void) {
    if (consume("{")) {
        Node head = {};
        Node *cur = &head;

        while (!consume("}")) {
            cur->next = stmt();
            cur = cur->next;
        }

        Node *node = new_node(ND_BLOCK);
        node->body = head.next;
        return node;
    }
    if (consume("return")) {
        Node *node = new_node(ND_RETURN);
        node->lhs = expr();
        expect(";");
        return node;
    }
    if (consume("if")) {
        Node *node = new_node(ND_IF);
        expect("(");
        node->lhs = expr();
        expect(")");
        node->rhs = stmt();

        if (consume("else")) {
            node->els = stmt();
        }
        return node;
    }
    if (consume("while")) {
        Node *node = new_node(ND_WHILE);
        expect("(");
        node->lhs = expr();
        expect(")");
        node->rhs = stmt();
        return node;
    }
    if (consume("for")) {
        Node *node = new_node(ND_FOR);
        expect("(");
        if (!consume(";")) {
            node->ini = new_node(ND_EXPR_STMT);
            node->ini->lhs = expr();
            expect(";");
        }
        if (!consume(";")) {
            node->lhs = expr();
            expect(";");
        }
        if (!consume(")")) {
            node->inc = new_node(ND_EXPR_STMT);
            node->inc->lhs = expr();
            expect(")");
        }
        node->rhs = stmt();
        return node;
    }

    Node *node = new_node(ND_EXPR_STMT);
    node->lhs = expr();
    expect(";");
    return node;
}

// expr = assign
static Node *expr(void) {
    return assign();
}

// assign = equality ("=" assign)?
static Node *assign(void) {
    Node *node = equality();
    if (consume("=")) {
        node = new_binary(ND_ASSIGN, node, assign());
    }
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(void) {
    Node *node = relational();

    for (;;) {
        if (consume("==")) {
            node = new_binary(ND_EQ, node, relational());
        } else if (consume("!=")) {
            node = new_binary(ND_NE, node, relational());
        } else {
            return node;
        }
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(void) {
    Node *node = add();

    for (;;) {
        if (consume("<")) {
            node = new_binary(ND_LT, node, add());
        } else if (consume("<=")) {
            node = new_binary(ND_LE, node, add());
        } else if (consume(">")) {
            node = new_binary(ND_LT, add(), node);
        } else if (consume(">=")) {
            node = new_binary(ND_LE, add(), node);
        } else {
            return node;
        }
    }
}

// add = mul ("+" mul | "-" mul)*
static Node *add(void) {
    Node *node = mul();

    for (;;) {
        if (consume("+")) {
            node = new_binary(ND_ADD, node, mul());
        } else if (consume("-")) {
            node = new_binary(ND_SUB, node, mul());
        } else {
            return node;
        }
    }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul(void) {
    Node *node = unary();

    for (;;) {
        if (consume("*")) {
            node = new_binary(ND_MUL, node, unary());
        } else if (consume("/")) {
            node = new_binary(ND_DIV, node, unary());
        } else {
            return node;
        }
    }
}

// unary = ("+" | "-")? unary
//       | primary
static Node *unary(void) {
    if (consume("+")) {
        return unary();
    } if (consume("-")) {
        return new_binary(ND_SUB, new_num(0), unary());
    }
    return primary();
}

// primary = "(" expr ")" | num
static Node *primary(void) {
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_ident();
    if (tok) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_LVAR;

        LVar *lvar = find_lvar(tok);
        if (lvar) {
            node->offset = lvar->offset;
        } else {
            lvar = calloc(1, sizeof(LVar));
            lvar->next   = locals;
            lvar->name   = tok->str;
            lvar->len    = tok->len;
            lvar->offset = locals ? locals->offset + 8 : 0;
            node->offset = lvar->offset;
            locals       = lvar;
        }
        return node;
    }

    return new_num(expect_number());
}

