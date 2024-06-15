#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_BUF 32768 // maximum size of buffer
#define MAX_LOG 20 // maximum height of the tree

typedef struct node {
    int *keys, *ties, m; // keys pointers + current key size
    struct node **kids, *next; // kids + next pointers
    int leaf; // is a leaf node
} node;

typedef node* pnode;
pnode root; // the root of the data structure
pnode *parents;
char *buffer; // to communicate with js
int degree, depth, timer; // timer to make ids unique

// constructor-like but in c
pnode create_node(int _leaf) {
    pnode cur = (pnode)malloc(sizeof(node));
    cur->m = 0;
    cur->leaf = _leaf;
    cur->next = NULL;
    cur->keys = (int*)malloc((degree-1) * sizeof(int));
    cur->ties = (int*)malloc((degree-1) * sizeof(int));
    if (_leaf) cur->kids = NULL;
    else cur->kids = (node**)malloc(degree * sizeof(node*));

    return cur;
}

// destroy a node and free memory
void free_node(pnode cur) {
    free(cur->keys);
    free(cur->ties);
    if (!cur->leaf)
        free(cur->kids);
}

void init(int _degree) {
    degree = _degree;
    depth = timer = 0;
    buffer = (char*)malloc(sizeof(char) * MAX_BUF);
    parents = (pnode*)malloc(sizeof(pnode*) * MAX_LOG);
    root = create_node(1);
}

// returns true if pair(key1, tie1)
// is smaller than pair(key2, tie2)
int comp(int key1, int tie1, int key2, int tie2) {
    if (key1 == key2)
        return (tie1 < tie2);
    return (key1 < key2);
}

pnode find_leaf(int key, int tie) {
    pnode cur = root;
    while (!cur->leaf) {
        parents[depth++] = cur;

        int i = 0;
        while (i < cur->m && !comp(key, tie, cur->keys[i], cur->ties[i]))
            i++; // find the first key[i] that is greater than key
        cur = cur->kids[i]; // set its pointer as the next node
    }

    return cur; // return the leaf node that should contain key
}

void print_tree(pnode cur, int dep) {
    if (cur->leaf) return;
    for (int i = 0 ; i <= cur->m; i++)
        print_tree(cur->kids[i], dep+1);
}

int find_key(pnode cur, int key) {
    // find the first key[i] that is greater than or equal to key
    int i = 0;
    while (i < cur->m && cur->keys[i] < key)
        i++;

    // check if equality is satisfied
    if (i < cur->m && key == cur->keys[i])
        return cur->ties[i];
    return 0;
}

pnode find(int key, int tie) {
    depth = 0;
    return find_leaf(key, tie);
}

void swap_int(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void swap_pnode(pnode *a, pnode *b) {
    pnode temp = *a;
    *a = *b;
    *b = temp;
}

void ins_par(pnode cur, int key, int tie, pnode nex) {
    // if (cur is the root of the tree
    if (cur == root) {
        root = create_node(0);
        root->m = 1;
        root->kids[0] = cur;
        root->keys[0] = key;
        root->ties[0] = tie;
        root->kids[1] = nex;
        return;
    }

    // take the parent of cur using the stack
    pnode par = parents[--depth];
    // find the kid[i] that is equal to cur
    int i = 0;
    while (par->kids[i] != cur)
        i++;

    // place nex in the right position
    while (i < par->m) {
        swap_int(&key, &par->keys[i]);
        swap_int(&tie, &par->ties[i]);
        swap_pnode(&nex, &par->kids[i+1]);
        i++;
    }

    // no neeed to split the parent node
    if (par->m < degree-1) {
        par->keys[par->m] = key;
        par->ties[par->m] = tie;
        par->kids[par->m+1] = nex;
        par->m += 1;
        return;
    }

    // we must split the parent node
    pnode sec = create_node(0);
    int bs = (degree+2)/2;
    int lm = degree - bs;
    while (sec->m < lm-1) {
        sec->keys[sec->m] = par->keys[bs + sec->m];
        sec->ties[sec->m] = par->ties[bs + sec->m];
        sec->kids[sec->m] = par->kids[bs + sec->m];
        sec->m += 1;
    }

    // add last values in sec
    sec->kids[sec->m] = par->kids[bs + sec->m];
    sec->keys[sec->m] = key;
    sec->ties[sec->m] = tie;
    sec->kids[sec->m+1] = nex;
    sec->m += 1;

    // reduce the size m of par
    par->m = bs-1;
    ins_par(par, par->keys[bs-1], par->ties[bs-1], sec);
}

void ins_leaf(pnode cur, int key, int tie) {
    // the leaf cur exists for sure
    int i = 0;
    while (i < cur->m && !comp(key, tie, cur->keys[i], cur->ties[i]))
        i++; // find the first key[i] that is greater than key

    while (i < cur->m) {
        swap_int(&key, &cur->keys[i]);
        swap_int(&tie, &cur->ties[i]);
        i++;
    }

    // no neeed to split the leaf node
    if (cur->m < degree-1) {
        cur->keys[cur->m] = key;
        cur->ties[cur->m] = tie;
        cur->m += 1;
        return;
    }

    // we must split the leaf node
    pnode nex = create_node(1);
    nex->next = cur->next;
    cur->next = nex;

    int bs = (degree+1)/2;
    int lm = degree - bs;
    while (nex->m < lm-1) {
        nex->keys[nex->m] = cur->keys[bs + nex->m];
        nex->ties[nex->m] = cur->ties[bs + nex->m];
        nex->m += 1;
    }

    // add the last values
    nex->keys[nex->m] = key;
    nex->ties[nex->m] = tie;
    nex->m += 1;

    // reduce the size m of cur
    cur->m = bs;

    ins_par(cur, nex->keys[0], nex->ties[0], nex);
}

void ins(int key) {
    int tie = ++timer;
    // find the leaf node to place the key
    pnode cur = find(key, tie);
    // always insert since timer is unique
    ins_leaf(cur, key, tie);
}

void del_par(pnode cur, pnode ptr) {
    // delete key from current node assuming that it exists
    int i = 0;
    while (cur->kids[i] != ptr)
        i++; // find the key[i] that is equal to ptr;
    i--;

    cur->m -= 1;
    while (i < cur->m) {
        cur->keys[i] = cur->keys[i+1];
        cur->ties[i] = cur->ties[i+1];
        cur->kids[i+1] = cur->kids[i+2];
        i++;
    }

    // in this implementation we never destroy the root
    if (cur == root && cur->m == 0) {
        root = cur->kids[0];
        free_node(cur); // destroy the node
        return;
    }

    // check whether it contains too few values
    if (cur == root || cur->m+1 >= (degree+1)/2)
        return; // the constraints are satisfied

    // find a neighbor child
    pnode par = parents[--depth], bro;
    i = 0;
    while (par->kids[i] != cur)
        i++;

    //with preference to the left
    // save the intermediate values of cur/bro
    int key, tie, before = 0;
    if (i > 0) {
        before = 1;
        bro = par->kids[i-1];
        key = par->keys[i-1];
        tie = par->ties[i-1];
    }
    else {
        bro = par->kids[i+1];
        key = par->keys[i];
        tie = par->ties[i];
    }

    if (cur->m + 1 + bro->m < degree) {
        // this is the merging case
        if (!before)
            swap_pnode(&cur, &bro);

        // copy contents of cur to bro
        bro->next = cur->next;
        bro->keys[bro->m] = key;
        bro->ties[bro->m] = tie;
        bro->m += 1;

        bro->kids[bro->m] = cur->kids[0];
        for (int i = 0; i < cur->m; i++) {
            bro->keys[bro->m] = cur->keys[i];
            bro->ties[bro->m] = cur->ties[i];
            bro->kids[bro->m+1] = cur->kids[i+1];
            bro->m += 1;
        }

        // delete cur from my father recursively
        free_node(cur); // destroy cur
        del_par(par, cur);
    }
    else {
        // this is the redistribution case
        if (before) { // bro comes before cur
            // move all the keys of cur to the right
            cur->m += 1;
            cur->kids[cur->m] = cur->kids[cur->m-1];
            for (int i = cur->m - 1; i > 0; i--) {
                cur->keys[i] = cur->keys[i-1];
                cur->ties[i] = cur->ties[i-1];
                cur->kids[i] = cur->kids[i-1];
            }

            // borrow a key from bro
            cur->keys[0] = key;
            cur->ties[0] = tie;
            cur->kids[0] = bro->kids[bro->m];

            // update bro and parent
            par->keys[i-1] = bro->keys[bro->m-1];
            par->ties[i-1] = bro->ties[bro->m-1];
            bro->m -= 1;
        }
        else { // cur comes before bro (the symmetric case)
            // borrow a key from bro
            cur->keys[cur->m] = key;
            cur->ties[cur->m] = tie;
            cur->kids[cur->m+1] = bro->kids[0];
            cur->m += 1;

            // update parent
            par->keys[i] = bro->keys[0];
            par->ties[i] = bro->ties[0];

            // move all the keys of bro to the left
            bro->m -= 1;
            bro->kids[0] = bro->kids[1];
            for (int i = 0; i < bro->m; i++) {
                bro->keys[i] = bro->keys[i+1];
                bro->ties[i] = bro->ties[i+1];
                bro->kids[i+1] = bro->kids[i+2];
            }
        }
    }
}

void del_leaf(pnode cur, int key, int tie) {
    // delete key from current node assuming that it exists
    int i = 0;
    while (comp(cur->keys[i], cur->ties[i], key, tie))
        i++; // find the first key[i] that is greater than or equal key

    cur->m -= 1;
    while (i < cur->m) {
        cur->keys[i] = cur->keys[i+1];
        cur->ties[i] = cur->ties[i+1];
        i++;
    }

    // check whether it contains too few values
    if (cur == root || cur->m >= degree/2)
        return; // the constraints are satisfied
    // in this implementation we never destroy the root

    // find a neighbor child
    pnode par = parents[--depth], bro = cur->next;
    i = 0;
    while (par->kids[i] != cur)
        i++;
    //with preference to the left
    if (i > 0) bro = par->kids[i-1];

    // save the intermediate values of cur/bro
    if (bro->next == cur) {
        key = par->keys[i-1];
        tie = par->ties[i-1];
    }
    else {
        key = par->keys[i];
        tie = par->ties[i];
    }

    if (cur->m + bro->m < degree) {
        // this is the merging case
        if (cur->next == bro)
            swap_pnode(&cur, &bro);

        // copy contents of cur to bro
        bro->next = cur->next;
        for (int i = 0; i < cur->m; i++) {
            bro->keys[bro->m] = cur->keys[i];
            bro->ties[bro->m] = cur->ties[i];
            bro->m += 1;
        }

        // delete cur from my father recursively
        free_node(cur); // destroy cur
        del_par(par, cur);
    }
    else {
        // this is the redistribution case
        if (bro->next == cur) { // bro comes before cur
            // move all the keys of cur to the right
            cur->m += 1;
            for (int i = cur->m - 1; i > 0; i--) {
                cur->keys[i] = cur->keys[i-1];
                cur->ties[i] = cur->ties[i-1];
            }

            // borrow a key from bro
            cur->keys[0] = bro->keys[bro->m];
            cur->ties[0] = bro->ties[bro->m];

            // update bro and parent
            par->keys[i-1] = cur->keys[0];
            par->ties[i-1] = cur->ties[0];
            bro->m -= 1;
        }
        else { // cur comes before bro (the symmetric case)
            // borrow a key from bro
            cur->keys[cur->m] = bro->keys[0];
            cur->ties[cur->m] = bro->ties[0];
            cur->m += 1;

            // move all the keys of bro to the left
            bro->m -= 1;
            for (int i = 0; i < bro->m; i++) {
                bro->keys[i] = bro->keys[i+1];
                bro->ties[i] = bro->ties[i+1];
            }

            // update parent
            par->keys[i] = bro->keys[0];
            par->ties[i] = bro->ties[0];
        }
    }
}

void del(int key) {
    int tie = 0;
    pnode cur = find(key, tie);
    if (!(tie = find_key(cur, key))) // if key found take the first appearance
        if (!(cur = cur->next) || !(tie = find_key(cur, key))) // else check the next node
            return; // else there so such key to delete and return

    // store the right parents stack
    find(key, tie);
    // delete starting from leaf
    del_leaf(cur, key, tie);
}

void push(char *buffer, int *po, char *str) {
    // Copy the string from source to destination
    while ((buffer[*po] = *str)) {
        str++;
        (*po)++;
    }
}

void push_int(char *buffer, int *po, int num) {
    if (num < 0) {
        push(buffer, po, "-");
        num *= -1;
    }

    int temp = num, sz = 0;
    do {
        sz++;
        temp /= 10;
    } while (temp);

    (*po) += sz;
    buffer[*po] = '\0';
    for (int i = 1; i <= sz; i++) {
        buffer[(*po) - i] = num%10 + '0';
        num /= 10;
    }
}

// Modified function to generate HTML and return as a string
void print_node(pnode cur, int depth, char *buffer, int *po) {
    if (cur != NULL) {
        push(buffer, po, "<li><a href=\"#\"><div class=\"row\">");
        for (int i = 0; i < cur->m; i++) {
            char keyStr[20];
            push(buffer, po, "<span>");
            push_int(buffer, po, cur->keys[i]);
            push(buffer, po, "</span>");
        }
        push(buffer, po, "</div></a>");
        if (!cur->leaf) {
            push(buffer, po, "\n<ul>\n");
            for (int i = 0; i <= cur->m; i++)
                print_node(cur->kids[i], depth+1, buffer, po);
            push(buffer, po, "</ul>\n");
        }
        push(buffer, po, "</li>\n");
    }
}

char *print() {
    buffer[0] = '\0';
    int val = 0;
    int *po = &val;

    print_node(root, 0, buffer, po);
    return buffer;
}

void rand_tree(int seed) {
    srand(seed);
    // choose between some values
    int iter = 10 + rand()%(8 + 4*degree);
    for (int i = 0; i < iter; i++) {
        // in the range of -100 and 100
        int key = -100 + rand()%201;
        ins(key);
    }
}

int main() {
    return 0;
}

EMSCRIPTEN_KEEPALIVE
void wasm_rand(int seed) {
    rand_tree(seed);
}

EMSCRIPTEN_KEEPALIVE
char *wasm_print() {
    return print();
}

// only create one root in the beginning of javascript!!!
EMSCRIPTEN_KEEPALIVE
void wasm_init(int deg) {
    init(deg);
}

EMSCRIPTEN_KEEPALIVE
void wasm_ins(int key) {
    ins(key);
}

EMSCRIPTEN_KEEPALIVE
void wasm_del(int key) {
    del(key);
}

EMSCRIPTEN_KEEPALIVE
void *wasm_malloc(size_t n) {
    return malloc(n);
}

EMSCRIPTEN_KEEPALIVE
void wasm_free(void *ptr) {
    free(ptr);
}
