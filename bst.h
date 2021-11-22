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
        std::mutex mtx;
        T val;
        node_t() {
            left = nullptr;
            right = nullptr;
        }

        node_t(const T& _val) {
            left = nullptr;
            right = nullptr;
            val = _val;
        }
    };
    node_t* root;
    std::mutex root_mtx;
    std::atomic<size_t> _size;
    void erase_helper(node_t* node, node_t *node_parent);
    node_t *find_left_max_p(node_t *node);
    void clear(node_t* node);
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
FineGrainedBST<T>::FineGrainedBST(): root(nullptr), _size(0) {}

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
    delete node;
    root = nullptr;
}

// template<typename T>
// bool FineGrainedBST<T>::insert(const T& t) {
//     std::pair<node_t*, char> fdir = find_helper(root, t);
//     node_t* dir;
//     if (fdir.second == 'L') {
//         dir = fdir.first->left;
//     } else {
//         dir = fdir.first->right;
//     }
//     bool inserted = false;
//     if (dir == nullptr) {
//         if (fdir.second == 'L') {
//             fdir.first->left = new node_t(t);
//         } else {
//             fdir.first->right = new node_t(t);
//         }
//         _size++;
//         inserted = true;
//     }
//     fdir.first->mtx.unlock();
//     return inserted;
// }

// template<typename T>
// std::pair<typename FineGrainedBST<T>::node_t*, char> FineGrainedBST<T>::find_helper(node_t* node, const T& element) const {
//     node_t* s;
//     char dir;
//     if (element < node->val) {
//         s = node->left;
//         dir = 'L';
//     } else {
//         s = node->right;
//         dir = 'R';
//     }
//     if (s != nullptr && s->val != element) {
//         return find_helper(s, element);
//     }
//     node->mtx.lock();
//     if (node->color == BLUE) {
//         node->mtx.unlock();
//         return find_helper(node->back, element);
//     } else {
//         bool changed = false;
//         if (dir == 'L') {
//             changed = node->left != s;
//         } else {
//             changed = node->right != s;
//         }
//         if (changed) {
//             node->mtx.unlock();
//             return find_helper(node, element);
//         } else {
//             return std::pair<node_t*, char>(node, dir);
//         }
//     }
// }

template<typename T>
bool FineGrainedBST<T>::insert(const T& t) {
    node_t *node = new node_t(t);
    root_mtx.lock();
    if (root == nullptr) {
        root = node;
        _size++;
        root_mtx.unlock();
        return true;
    } else {
        node_t *parent = root;
        node_t *cur;
        if (t == parent->val) {
            root_mtx.unlock();
            return false;
        } else if (t < parent->val) {
            cur = parent->left;
        } else {
            cur = parent->right;
        }
        parent->mtx.lock();
        root_mtx.unlock();
        if (cur != nullptr) cur->mtx.lock();
        while (cur != nullptr) {
            node_t *old_parent = parent;
            parent = cur;
            if (t == parent->val) {
                old_parent->mtx.unlock();
                parent->mtx.unlock();
                return false;
            } else if (t < parent->val) {
                cur = parent->left;
            } else {
                cur = parent->right;
            }
            old_parent->mtx.unlock();
            if (cur != nullptr) cur->mtx.lock();
        }
        if (t < parent->val) {
            parent->left = node;
        } else {
            parent->right = node;
        }
        parent->mtx.unlock();
        _size++;
        return true;
    }
}

template<typename T>
typename FineGrainedBST<T>::node_t* FineGrainedBST<T>::find_left_max_p(node_t *node) {
    node_t *parent = node;
    node_t *cur = parent->left;
    if (cur != nullptr) cur->mtx.lock();
    while (cur->right != nullptr) {
        node_t *old_parent = parent;
        parent = cur;
        cur = parent->right;
        if (old_parent != node) old_parent->mtx.unlock();
        if (cur != nullptr) cur->mtx.unlock();
    }
    printf("find left max parent: %d, replace: %d\n", (int)parent->val, (int)cur->val);
    return parent;
}

template<typename T>
void FineGrainedBST<T>::erase_helper(node_t *node, node_t *node_parent) {
    // If node is leaf
    if (node->left == nullptr && node->right == nullptr) {
        printf("erase case 1\n");
        if (node_parent == nullptr) {
            root = nullptr;
        } else if (node->val < node_parent->val) {
            node_parent->left = nullptr;
        } else {
            node_parent->right = nullptr;
        }
        delete(node);
    }
    // If node has no right child
    else if (node->right == nullptr) {
        printf("erase case 2\n");
        if (node_parent == nullptr) {
            root = node->left;
        } else if (node->val < node_parent->val) {
            node_parent->left = node->left;
        } else {
            node_parent->right = node->left;
        }
        delete(node);
    }
    // If node has no left child
    else if (node->left == nullptr) {
        printf("erase case 3\n");
        // printf("no left child\n");
        if (node_parent == nullptr) {
            root = node->right;
        } else if (node->val < node_parent->val) {
            node_parent->left = node->right;
        } else {
            node_parent->right = node->right;
        }
        delete(node);
        // printf("node deleted\n");
    }
    // If node has two children
    // Replace node's value with the largest value in left subtree
    else {
        printf("erase case 4\n");
        node_t *left_max_p = find_left_max_p(node);
        if (left_max_p == node) {
            node_t *replace = node->left;
            node->val = replace->val;
            printf("replaced 1: %d\n", (int)node->val);
            node->left = replace->left;
            replace->mtx.unlock();
            delete(replace);
        } else {
            node_t *replace = left_max_p->right;
            node->val = replace->val;
            printf("replaced 2: %d\n", (int)node->val);
            left_max_p->right = replace->left;
            left_max_p->mtx.unlock();
            replace->mtx.unlock();
            delete(replace);
        }
    }
    if (node_parent != nullptr) node_parent->mtx.unlock();
    node->mtx.unlock();
    // printf("node unlocked\n");
}

template<typename T>
void FineGrainedBST<T>::erase(const T& t) {
    printf("erase %d\n", (int)t);
    if (!find(t)) {
        return;
    }
    root_mtx.lock();
    printf("root lock locked\n");
    printf("root node value %d\n", (int)root->val);
    // if (root == nullptr) {
    //     root_mtx.unlock();
    //     return;
    // }
    node_t *parent = root;
    node_t *cur;
    if (t == parent->val) {
        printf("erase root\n");
        parent->mtx.lock();
        printf("root node locked\n");
        erase_helper(parent, nullptr);
        root_mtx.unlock();
        _size--;
        return;
    } else if (t < parent->val) {
        cur = parent->left;
    } else {
        cur = parent->right;
    }
    parent->mtx.lock();
    root_mtx.unlock();
    if (cur != nullptr) cur->mtx.lock();
    while (cur != nullptr) {
        if (cur->val == t) {
            erase_helper(cur, parent);
            _size--;
            return;
        }
        node_t *old_parent = parent;
        parent = cur;
        if (t < parent->val) {
            cur = parent->left;
        } else {
            cur = parent->right;
        }
        old_parent->mtx.unlock();
        if (cur != nullptr) cur->mtx.lock();
    }
    parent->mtx.unlock();
}

template<typename T>
bool FineGrainedBST<T>::find(const T& t) {
    root_mtx.lock();
    if (root == nullptr) {
        root_mtx.unlock();
        return false;
    } else {
        node_t *parent = root;
        node_t *cur;
        if (t == parent->val) {
            root_mtx.unlock();
            return true;
        } else if (t < parent->val) {
            cur = parent->left;
        } else {
            cur = parent->right;
        }
        parent->mtx.lock();
        root_mtx.unlock();
        if (cur != nullptr) cur->mtx.lock();
        while (cur != nullptr) {
            node_t *old_parent = parent;
            parent = cur;
            if (t == parent->val) {
                old_parent->mtx.unlock();
                parent->mtx.unlock();
                return true;
            } else if (t < parent->val) {
                cur = parent->left;
            } else {
                cur = parent->right;
            }
            old_parent->mtx.unlock();
            if (cur != nullptr) cur->mtx.lock();
        }
        parent->mtx.unlock();
        return false;
    }
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
