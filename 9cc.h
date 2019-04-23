
// トークンの型を表す値
enum {
    TK_NUM = 256,  // 整数トークン
    TK_EOF,        // 入力の終わりを表すトークン
};

enum {
    ND_NUM = 256,  // 整数のノードの型:
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
} Node;

// トークンの型
typedef struct {
    int ty;       // トークンの型
    int val;      // tyがTK_NUMの場合, その数値
    char *input;  // トークン文字列 (エラーメッセージ用)
} Token;


// プロトタイプ宣言
Node *add();
Vector *new_vector();
