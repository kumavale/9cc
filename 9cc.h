
// トークンの型を表す値
enum {
    TK_NUM = 256,  // 整数トークン
    TK_IDENT,      // 識別子
    TK_RETURN,     // return
    TK_EOF,        // 入力の終わりを表すトークン
};

enum {
    ND_NUM = 256,  // 整数のノードの型:
    ND_IDENT,      // 識別子のノードの型
    ND_RETURN,     // returnのノードの型
};

typedef struct {
    void **data;
    int capacity;
    int len;
} Vector;

typedef struct {
    int ty;            // 演算子かND_NUM
    struct Node *lhs;  // 左辺
    struct Node *rhs;  // 右辺
    int val;           // tyがND_NUMの場合のみ使用
    char name;         // tyがND_IDENTの場合のみ使用
} Node;

// トークンの型
typedef struct {
    int ty;       // トークンの型
    int val;      // tyがTK_NUMの場合, その数値
    char *input;  // トークン文字列 (エラーメッセージ用)
} Token;

// マップの連想配列
typedef struct {
    Vector *keys;
    Vector *vals;
} Map;


// プロトタイプ宣言
Node *add();
Vector *new_vector();
void vec_push(Vector *vec, void *elem);
Node *new_node(int ty, Node *lhs, Node *rhs);
int consume(int ty);
void error(char *fmt, ...);
