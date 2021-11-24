#ifndef BST_H
#define BST_H

#include "hazard.h"
#include <stdio.h>
#include <mutex>
#include <atomic>
#include <cstring>
#include <vector>
#include <memory>

template<typename T>
class BST {
public:
    virtual bool insert(const T& t)=0;
    virtual void erase(const T& t)=0;
    virtual bool find(const T& t)=0;
    virtual size_t size()=0;
    virtual void clear()=0;
};

template<typename T>
class CoarseGrainedBST : public BST<T> {
    struct node_t {
        node_t* left;
        node_t* right;
        T val;
        node_t(const T& _val) {
            left = nullptr;
            right = nullptr;
            val = _val;
        }
    };
    node_t* root;
    size_t _size;
    std::mutex mtx;
    bool insert_helper(node_t* node, const T& elemnt);
    bool find_helper(const node_t* node, const T& element) const;
    void erase_helper(node_t* parent, node_t* node, const T& element);
    void clear(node_t* node);
public:
    CoarseGrainedBST();
    virtual ~CoarseGrainedBST();
    CoarseGrainedBST(const CoarseGrainedBST& other)=delete;
    CoarseGrainedBST& operator=(const CoarseGrainedBST& other)=delete;
    CoarseGrainedBST(const CoarseGrainedBST&& other)=delete;
    CoarseGrainedBST& operator=(const CoarseGrainedBST&& other)=delete;
    virtual bool insert(const T& t);
    virtual void erase(const T& t);
    virtual bool find(const T& t);
    virtual size_t size();
    virtual void clear();
};

template<typename T>
CoarseGrainedBST<T>::CoarseGrainedBST(): root(nullptr), _size(0) {}

template<typename T>
CoarseGrainedBST<T>::~CoarseGrainedBST() {
    clear();
}

template<typename T>
void CoarseGrainedBST<T>::clear() {
    clear(root);
    root = nullptr;
}

template<typename T>
void CoarseGrainedBST<T>::clear(node_t* node) {
    if (node == nullptr) {
        return;
    }
    clear(node->left);
    clear(node->right);
    delete node;
}

template<typename T>
bool CoarseGrainedBST<T>::insert(const T& t) {
    mtx.lock();
    if (root == nullptr) {
        root = new node_t(t);
        _size++;
        mtx.unlock();
        return true;
    }
    bool inserted = insert_helper(root, t);
    if (inserted) {
        _size++;
    }
    mtx.unlock();
    return inserted;
}

template<typename T>
bool CoarseGrainedBST<T>::insert_helper(node_t* node, const T& element) {
    const T& node_val = node->val;
    node_t* left = node->left;
    node_t* right = node->right;
    bool inserted = false;
    if (element == node_val) {
        inserted = false;
    } else if (element < node_val) {
        if (left == nullptr) {
            node_t* new_node = new node_t(element);
            node->left = new_node;
            inserted = true;
        } else {
            inserted = insert_helper(left, element);
        }
    } else {
        if (right == nullptr) {
            node_t* new_node = new node_t(element);
            node->right = new_node;
            inserted = true;
        } else {
            inserted = insert_helper(right, element);
        }
    }
    return inserted;
}

template<typename T>
void CoarseGrainedBST<T>::erase(const T& t) {
    mtx.lock();
    erase_helper(root, root, t);
    mtx.unlock();
}

template<typename T>
void CoarseGrainedBST<T>::erase_helper(node_t* parent, node_t* node, const T& element) {
    if (node == nullptr) {
        return;
    }
    const T& val = node->val;
    node_t* left = node->left;
    node_t* right = node->right;
    node_t* parent_left = parent->left;
    node_t* parent_right = parent->right;
    if (element == val) {
        node_t* neighbor = node->left;
        node_t* neighbor_parent = node;
        node_t* neighbor_left = nullptr;
        node_t* neighbor_right = nullptr;
        while (neighbor != nullptr && neighbor->right != nullptr) {
            neighbor_parent = neighbor;
            neighbor = neighbor->right;
        }
        if (neighbor == nullptr) {
            neighbor = node->right;
            neighbor_parent = node;
            while (neighbor != nullptr && neighbor->left != nullptr) {
                neighbor_parent = neighbor;
                neighbor = neighbor->left;
            }
        }
        if (neighbor != nullptr) {
            neighbor_left = neighbor->left;
            neighbor_right = neighbor->right;
        }
        node_t* neighbor_parent_left = neighbor_parent->left;
        node_t* neighnor_parent_right = neighbor_parent->right;
        if (parent_left == node) {
            parent->left = neighbor;
        } else if (parent_right == node) {
            parent->right = neighbor;
        }
        if (neighbor != nullptr) {
            if (left == neighbor) {
                left = neighbor_left;
            }
            neighbor->left = left;
            if (right == neighbor) {
                right = neighbor_right;
            }
            neighbor->right = right;
        }
        if (neighbor_parent_left == neighbor) {
            neighbor_parent->left = neighbor_right;
        }
        if (neighnor_parent_right == neighbor) {
            neighbor_parent->right = neighbor_left;
        }
        if (node == root) {
            root = neighbor;
        }
        _size--;
        delete node;
    } else if (element < val) {
        erase_helper(node, node->left, element);
    } else {
        erase_helper(node, node->right, element);
    }
}

template<typename T>
bool CoarseGrainedBST<T>::find(const T& t) {
    mtx.lock();
    bool found = find_helper(root, t);
    mtx.unlock();
    return found;
}

template<typename T>
bool CoarseGrainedBST<T>::find_helper(const node_t* node, const T& element) const {
    if (node == nullptr) {
        return false;
    }
    bool found = false;
    const T& val = node->val;
    if (element == val) {
        found = true;
    } else if (element < val) {
        found = find_helper(node->left, element);
    } else {
        found = find_helper(node->right, element);
    } 
    return found;
}

template<typename T>
size_t CoarseGrainedBST<T>::size() {
    return _size;
}

template<typename T>
class FineGrainedBST : public BST<T> {
    enum Dir {
        Left=0, Right=1
    };
    enum Color {
        White, Blue
    };
    struct node_t {
        node_t* children[2];
        node_t* back;
        std::mutex mtx;
        T val;
        Color color;
        node_t() {
            init();
        }

        node_t(const T& _val) {
            init();
            val = _val;
        }

        void init() {
            children[Dir::Left] = nullptr;
            children[Dir::Right] = nullptr;
            back = nullptr;
            memset(&val, 0xff, sizeof(T));
            color = Color::White;
        }
    };
    node_t* root;
    std::atomic<size_t> _size;
    std::pair<node_t*, Dir> find_helper(node_t* node, const T& element) const;
    std::vector<node_t*> rotation(node_t* a, Dir dir1, Dir dir2);
    void clear(node_t* node);
    void remove(typename FineGrainedBST<T>::node_t* a, Dir dir1, Dir dir2);
    void deletion_by_rotation(node_t* f, Dir dir);
public:
    FineGrainedBST();
    virtual ~FineGrainedBST();
    FineGrainedBST(FineGrainedBST& other)=delete;
    FineGrainedBST(FineGrainedBST&& other)=delete;
    FineGrainedBST& operator=(const FineGrainedBST& other)=delete;
    FineGrainedBST& operator=(const FineGrainedBST&& other)=delete;
    virtual bool insert(const T& t);
    virtual void erase(const T& t);
    virtual bool find(const T& t);
    virtual size_t size();
    virtual void clear();
};

template<typename T>
FineGrainedBST<T>::FineGrainedBST(): root(new node_t()), _size(0) {}

template<typename T>
FineGrainedBST<T>::~FineGrainedBST() {
    clear();
    delete root;
}

template<typename T>
void FineGrainedBST<T>::clear() {
    clear(root);
    root = new node_t();
}

template<typename T>
void FineGrainedBST<T>::clear(node_t* node) {
    if (node == nullptr) {
        return;
    }
    clear(node->children[Dir::Left]);
    clear(node->children[Dir::Right]);
    delete node;
}

template<typename T>
bool FineGrainedBST<T>::insert(const T& t) {
    std::pair<node_t*, Dir> fdir = find_helper(root, t);
    node_t* parent = fdir.first;
    append_hp(parent);
    append_hp(parent->children[Dir::Left]);
    append_hp(parent->children[Dir::Right]);
    Dir dir = fdir.second;
    node_t* child = parent->children[dir];
    bool inserted = false;
    if (child == nullptr) {
        parent->children[dir] = new node_t(t);
        _size++;
        inserted = true;
    }
    parent->mtx.unlock();
    return inserted;
}

template<typename T>
std::pair<typename FineGrainedBST<T>::node_t*, typename FineGrainedBST<T>::Dir> FineGrainedBST<T>::find_helper(node_t* node, const T& element) const {
    Dir dir;
    append_hp(node);
    append_hp(node->children[Dir::Left]);
    append_hp(node->children[Dir::Right]);
    if (element < node->val) {
        dir = Dir::Left;
    } else {
        dir = Dir::Right;
    }
    node_t* child = node->children[dir];
    if (child != nullptr && child->val != element) {
        return find_helper(child, element);
    }
    node->mtx.lock();
    if (node->color == Color::Blue) {
        node->mtx.unlock();
        return find_helper(node->back, element);
    } else if (node->children[dir] != child) {
        node->mtx.unlock();
        return find_helper(node, element);
    }
    return std::pair<node_t*, Dir>(node, dir);
}

template<typename T>
void FineGrainedBST<T>::erase(const T& t) {
    std::pair<node_t*, Dir> fdir = find_helper(root, t);
    node_t* parent = fdir.first;
    append_hp(parent);
    append_hp(parent->children[Dir::Left]);
    append_hp(parent->children[Dir::Right]);
    Dir dir = fdir.second;
    node_t* child = parent->children[dir];
    if (child == nullptr) {
        parent->mtx.unlock();
    } else {
        child->mtx.lock();
        deletion_by_rotation(parent, dir);
        _size--;
    }
}

template<typename T>
void FineGrainedBST<T>::deletion_by_rotation(typename FineGrainedBST<T>::node_t* f, Dir dir) {
    append_hp(f);
    append_hp(f->children[Dir::Left]);
    append_hp(f->children[Dir::Right]);
    node_t* s = f->children[dir];
    if (s->children[Dir::Left] == nullptr) {
        remove(f, dir, Dir::Right);
    } else {
        std::vector<node_t*> fgh = rotation(f, dir, Dir::Left);
        f = fgh[0];
        node_t* g = fgh[1];
        node_t* h = fgh[2];
        append_hp(f);
        append_hp(g);
        append_hp(h);
        if (h->children[Dir::Left] == nullptr) {
            deletion_by_rotation(g, Dir::Right);
        } else {
            deletion_by_rotation(g, Dir::Right);
            append_hp(f);
            append_hp(g);
            f->mtx.lock();
            if (g != f->children[dir] || f->color == Color::Blue) {
                f->mtx.unlock();
            } else {
                g->mtx.lock();
                std::vector<node_t*> fgh_new = rotation(f, dir, Dir::Right);
                f = fgh_new[0];
                append_hp(f);
                node_t* g_new = fgh_new[1];
                append_hp(g_new);
                node_t* h_new = fgh_new[2];
                append_hp(h_new);
                g_new->mtx.unlock();
                h_new->mtx.unlock();
            }
        }
    }
}

template<typename T>
void FineGrainedBST<T>::remove(typename FineGrainedBST<T>::node_t* a, Dir dir1, Dir dir2) {
    append_hp(a);
    node_t* b = a->children[dir1];
    append_hp(b);
    node_t* c = b->children[dir2];
    a->children[dir1] = c;
    b->children[dir2] = c;
    b->back = a;
    b->color = Color::Blue;
    retire<node_t>(b);
    a->mtx.unlock();
    b->mtx.unlock();
}

template<typename T>
std::vector<typename FineGrainedBST<T>::node_t*> FineGrainedBST<T>::rotation(node_t* a, Dir dir1, Dir dir2) {
    append_hp(a);
    node_t* b = a->children[dir1];
    append_hp(b);
    node_t* c = b->children[dir2];
    append_hp(c);
    node_t* b_new = new node_t();
    append_hp(b_new);
    node_t* c_new = new node_t();
    append_hp(c_new);
    
    b_new->mtx.lock();
    c_new->mtx.lock();
    c->mtx.lock();
    
    c_new->children[dir2] = c->children[dir2];
    c_new->children[dir2 ^ 1] = b_new;
    c_new->val = c->val;
    b_new->children[dir2] = c->children[dir2 ^ 1];
    b_new->children[dir2 ^ 1] = b->children[dir2 ^ 1];
    b_new->val = b->val;
    a->children[dir1] = c_new;
    
    b->back = a;
    b->color = Color::Blue;
    retire<node_t>(b);
    c->back = c_new;
    c->color = Color::Blue;
    retire<node_t>(c);
    a->mtx.unlock();
    b->mtx.unlock();
    c->mtx.unlock();
    return { a, c_new, b_new };
}

template<typename T>
bool FineGrainedBST<T>::find(const T& t) {
    std::pair<node_t*, Dir> fdir = find_helper(root, t);
    node_t* parent = fdir.first;
    append_hp(parent);
    Dir dir = fdir.second;
    node_t* child = parent->children[dir];
    bool found = child != nullptr;
    parent->mtx.unlock();
    return found;
}

template<typename T>
size_t FineGrainedBST<T>::size() {
    return _size.load();
}

template<typename T>
class LockFreeBST : public BST<T> {
public:
    virtual bool insert(const T& t)=0;
    virtual void erase(const T& t)=0;
    virtual bool find(const T& t)=0;
    virtual size_t size()=0;
    virtual void clear()=0;
};

template<typename T>
bool LockFreeBST<T>::insert(const T& t) {
    return false;
}

template<typename T>
void LockFreeBST<T>::erase(const T& t) {
    
}

template<typename T>
bool LockFreeBST<T>::find(const T& t) {
    return false;
}

template<typename T>
size_t size() {
    return 0;
}

template<typename T>
void clear() {

}

#endif
