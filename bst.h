#ifndef BST_H
#define BST_H

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
        std::shared_ptr<node_t> children[2];
        std::shared_ptr<node_t> back;
        std::mutex mtx;
        T val;
        Color color;
        node_t() {
            memset(&val, 0, sizeof(T));
            color = Color::White;
        }

        node_t(const T& _val) {
            val = _val;
            color = Color::White;
        }
    };
    std::shared_ptr<node_t> root;
    std::atomic<size_t> _size;
    std::pair<std::shared_ptr<node_t>, Dir> find_helper(std::shared_ptr<node_t>& node, const T& element) const;
    std::vector<std::shared_ptr<node_t>> rotation(std::shared_ptr<node_t>& a, Dir dir1, Dir dir2);
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
FineGrainedBST<T>::FineGrainedBST(): root(std::make_shared<node_t>()), _size(0) {}

template<typename T>
FineGrainedBST<T>::~FineGrainedBST() {
    clear();
}

template<typename T>
void FineGrainedBST<T>::clear() {
    root = std::make_shared<node_t>();
}

template<typename T>
bool FineGrainedBST<T>::insert(const T& t) {
    std::pair<std::shared_ptr<node_t>, Dir> fdir = find_helper(root, t);
    std::shared_ptr<node_t> parent = fdir.first;
    Dir dir = fdir.second;
    std::shared_ptr<node_t> child = parent->children[dir];
    bool inserted = false;
    if (child == nullptr) {
        parent->children[dir] = std::make_shared<node_t>(t);
        _size++;
        inserted = true;
    }
    parent->mtx.unlock();
    return inserted;
}

template<typename T>
std::pair<std::shared_ptr<typename FineGrainedBST<T>::node_t>, typename FineGrainedBST<T>::Dir> 
FineGrainedBST<T>::find_helper(std::shared_ptr<node_t>& node, const T& element) const {
    Dir dir;
    if (element < node->val) {
        dir = Dir::Left;
    } else {
        dir = Dir::Right;
    }
    std::shared_ptr<node_t> child = node->children[dir];
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
    return std::pair<std::shared_ptr<node_t>, Dir>(node, dir);
}

template<typename T>
void FineGrainedBST<T>::erase(const T& t) {
    
}

template<typename T>
std::vector<std::shared_ptr<typename FineGrainedBST<T>::node_t>> FineGrainedBST<T>::rotation(std::shared_ptr<node_t>& a, Dir dir1, Dir dir2) {
    std::shared_ptr<node_t> b = a->children[dir1];
    std::shared_ptr<node_t> c = b->children[dir2];
    std::shared_ptr<node_t> b_new = std::make_shared<node_t>();
    std::shared_ptr<node_t> c_new = std::make_shared<node_t>();
    
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
    c->back = c_new;
    c->color = Color::Blue;
    a->mtx.unlock();
    b->mtx.unlock();
    c->mtx.unlock();
    return { a, c_new, b_new };
}

template<typename T>
bool FineGrainedBST<T>::find(const T& t) {
    std::pair<std::shared_ptr<node_t>, Dir> fdir = find_helper(root, t);
    std::shared_ptr<node_t> parent = fdir.first;
    Dir dir = fdir.second;
    std::shared_ptr<node_t> child = parent->children[dir];
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
