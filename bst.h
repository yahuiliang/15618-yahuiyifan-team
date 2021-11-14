#ifndef BST_H
#define BST_H

#include <stdio.h>
#include <mutex>
#include <atomic>
#include <vector>

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
}

template<typename T>
void CoarseGrainedBST<T>::clear(node_t* node) {
    if (node == nullptr) {
        return;
    }
    clear(node->left);
    clear(node->right);
    delete node;
    root = nullptr;
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
    struct node_t {
        node_t* left;
        node_t* right;
        node_t* back;
        bool deleted;
        std::mutex mtx;
        T val;
        node_t() {
            left = nullptr;
            right = nullptr;
            back = nullptr;
            deleted = false;
        }

        node_t(const T& _val) {
            left = nullptr;
            right = nullptr;
            back = nullptr;
            deleted = false;
            val = _val;
        }
    };
    node_t* root;
    std::mutex root_mtx;
    std::atomic<size_t> _size;
    std::vector<node_t *> gc_queue;
    std::mutex gc_mtx;
    std::pair<node_t*, char> find_helper(node_t* node, const T& element) const;
    std::pair<node_t *, node_t *> rotation(node_t* a, char dir1, char dir2);
    void deletion_by_rotation(node_t *f, char dir);
    void remove(node_t *a, char dir1, char dir2);
    void clear(node_t* node);
    void clear_gc();
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
}

template<typename T>
void FineGrainedBST<T>::clear() {
    clear(root);
}

template<typename T>
void FineGrainedBST<T>::clear(node_t* node) {
    if (node == nullptr) {
        return;
    }
    clear(node->left);
    clear(node->right);
    clear(node->back);
    delete node;
    root = new node_t();
}

template<typename T>
void FineGrainedBST<T>::clear_gc() {
    for (size_t i = 0; i < gc_queue.size(); i++) {
        delete(gc_queue[i]);
    }
    gc_queue.clear();
}

template<typename T>
bool FineGrainedBST<T>::insert(const T& t) {
    std::pair<node_t*, char> fdir = find_helper(root, t);
    node_t* dir;
    if (fdir.second == 'L') {
        dir = fdir.first->left;
    } else {
        dir = fdir.first->right;
    }
    bool inserted = false;
    if (dir == nullptr) {
        if (fdir.second == 'L') {
            fdir.first->left = new node_t(t);
        } else {
            fdir.first->right = new node_t(t);
        }
        _size++;
        inserted = true;
    }
    fdir.first->mtx.unlock();
    return inserted;
}

template<typename T>
std::pair<typename FineGrainedBST<T>::node_t*, char> FineGrainedBST<T>::find_helper(node_t* node, const T& element) const {
    // printf("element to find: %d\n", (int)element);
    // printf("find node: %d\n", (int)node->val);
    node_t* s;
    char dir;
    if (element < node->val) {
        s = node->left;
        dir = 'L';
    } else {
        s = node->right;
        dir = 'R';
    }
    if (s != nullptr) {
        // printf("s_val: %d\n", (int)s->val);
    }
    if (s != nullptr && s->val != element) {
        return find_helper(s, element);
    }
    node->mtx.lock();
    if (node->deleted == true) {
        node->mtx.unlock();
        return find_helper(node->back, element);
    } else {
        bool changed = false;
        if (dir == 'L') {
            changed = node->left != s;
        } else {
            changed = node->right != s;
        }
        if (changed) {
            node->mtx.unlock();
            return find_helper(node, element);
        } else {
            return std::pair<node_t*, char>(node, dir);
        }
    }
}

template<typename T>
bool FineGrainedBST<T>::find(const T& t) {
    std::pair<node_t*, char> fdir = find_helper(root, t);
    node_t* dir;
    if (fdir.second == 'L') {
        dir = fdir.first->left;
    } else {
        dir = fdir.first->right;
    }
    bool found = dir != nullptr;
    fdir.first->mtx.unlock();
    return found;
}

template<typename T>
std::pair<typename FineGrainedBST<T>::node_t *, typename FineGrainedBST<T>::node_t *> FineGrainedBST<T>::rotation(node_t* a, char dir1, char dir2) {
    node_t *b;
    if (dir1 == 'L') {
        b = a->left;
    } else {
        b = a->right;
    }
    node_t *c;
    if (dir2 == 'L') {
        c = b->left;
    } else {
        c = b->right;
    }
    node_t *b_p = new node_t();
    node_t *c_p = new node_t();
    c_p->mtx.lock();
    b_p->mtx.lock();
    c->mtx.lock();
    if (dir2 == 'L') {
        c_p->left = c->left;
        c_p->right = b_p;
        c_p->val = c->val;
        b_p->left = c->right;
        b_p->right = b->right;
        b_p->val = b->val;
    } else {
        c_p->right = c->right;
        c_p->left = b_p;
        c_p->val = c->val;
        b_p->right = c->left;
        b_p->left = b->left;
        b_p->val = b->val;
    }
    if (dir1 == 'L') {
        a->left = c_p;
    } else {
        a->right = c_p;
    }
    b->back = a;
    b->deleted = true;
    c->back = c_p;
    c->deleted = true;
    gc_mtx.lock();
    gc_queue.push_back(b);
    gc_queue.push_back(c);
    gc_mtx.unlock();
    a->mtx.unlock();
    b->mtx.unlock();
    c->mtx.unlock();
    return std::pair<node_t *, node_t *>(c_p, b_p);
}

template<typename T>
void FineGrainedBST<T>::remove(node_t *a, char dir1, char dir2) {
    node_t *b;
    if (dir1 == 'L') {
        b = a->left;
    } else {
        b = a->right;
    }
    node_t *c;
    if (dir2 == 'L') {
        c = b->left;
    } else {
        c = b->right;
    }
    if (dir1 == 'L') {
        a->left = c;
    } else {
        a->right = c;
    }
    if (dir2 == 'L') {
        b->right = c;
    } else {
        b->left = c;
    }
    b->back = a;
    b->deleted = true;
    gc_mtx.lock();
    gc_queue.push_back(b);
    gc_mtx.unlock();
    a->mtx.unlock();
    b->mtx.unlock();
}

template<typename T>
void FineGrainedBST<T>::deletion_by_rotation(node_t *f, char dir) {
    node_t *s;
    if (dir == 'L') {
        s = f->left;
    } else {
        s = f->right;
    }
    if (s->left == nullptr) {
        remove(f, dir, 'R');
    } else {
        // printf("rotate %d, dir1: %c, dir2: L\n", (int)f->val, dir);
        std::pair<node_t*, node_t*> g_h = rotation(f, dir, 'L');
        node_t *g = g_h.first;
        node_t *h = g_h.second;
        if (h->left == nullptr) {
            deletion_by_rotation(g, 'R');
        } else {
            deletion_by_rotation(g, 'R');
            f->mtx.lock();
            bool rebalance_failed = false;
            if (dir == 'L') {
                if (g != f->left) {
                    rebalance_failed = true;
                }
            } else {
                if (g != f->right) {
                    rebalance_failed = true;
                }
            }
            if (f->deleted == true) {
                rebalance_failed = true;
            }
            if (rebalance_failed) {
                f->mtx.unlock();
            } else {
                g->mtx.lock();
                std::pair<node_t*, node_t*> gp_hp = rotation(f, dir, 'R');
                gp_hp.first->mtx.unlock();
                gp_hp.second->mtx.unlock();
            }
        }
    }
}

template<typename T>
void FineGrainedBST<T>::erase(const T& t) {
    // printf("erase node: %d\n", (int)t);
    std::pair<node_t*, char> f_dir = find_helper(root, t);
    node_t *f = f_dir.first;
    char dir = f_dir.second;
    // printf("f node: %d\n", (int)f->val);
    // printf("dir: %c\n", dir);
    if (dir == 'L') {
        // value not in tree
        if (f->left == nullptr) {
            f->mtx.unlock();
            return;
        }
    } else {
        if (f->right == nullptr) {
            f->mtx.unlock();
            return;
        }
    }
    node_t *s;
    if (dir == 'L') {
        s = f->left;
    } else {
        s = f->right;
    }
    s->mtx.lock();
    deletion_by_rotation(f, dir);
    _size--;
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
