#ifndef BST_H
#define BST_H

#include <stdio.h>

template<typename T>
class BST {
protected:
    struct Node {
        Node* left;
        Node* right;
        T val;
        Node(const T& _val) {
            left = nullptr;
            right = nullptr;
            val = _val;
        }
    };
public:
    virtual bool insert(const T& t)=0;
    virtual void erase(const T& t)=0;
    virtual bool find(const T& t) const=0;
    virtual size_t size()=0;
};

template<typename T>
class CoarseGrainedBST : public BST<T> {
    typedef typename BST<T>::Node node_t;
    node_t* root;
    size_t _size;
    bool insert_helper(node_t* node, const T& elemnt);
    bool find_helper(const node_t* node, const T& element) const;
    void erase_helper(node_t* parent, node_t* node, const T& element);
    void free_helper(node_t* node);
public:
    CoarseGrainedBST();
    virtual ~CoarseGrainedBST();
    virtual bool insert(const T& t);
    virtual void erase(const T& t);
    virtual bool find(const T& t) const;
    virtual size_t size();
};

template<typename T>
CoarseGrainedBST<T>::CoarseGrainedBST(): root(nullptr), _size(0) {}

template<typename T>
CoarseGrainedBST<T>::~CoarseGrainedBST() {
    free_helper(root);
}

template<typename T>
void CoarseGrainedBST<T>::free_helper(node_t* node) {
    if (node == nullptr) {
        return;
    }
    free_helper(node->left);
    free_helper(node->right);
    delete node;
}

template<typename T>
bool CoarseGrainedBST<T>::insert(const T& t) {
    if (root == nullptr) {
        root = new node_t(t);
        _size++;
        return true;
    }
    bool inserted = insert_helper(root, t);
    if (inserted) {
        _size++;
    }
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
    erase_helper(root, root, t);
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
bool CoarseGrainedBST<T>::find(const T& t) const {
    return find_helper(root, t);
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
    typename BST<T>::Node* root;
public:
    FineGrainedBST();
    virtual void insert(const T& t);
    virtual void erase(const T& t);
    virtual void find(const T& t);
    virtual size_t size();
};

template<typename T>
void FineGrainedBST<T>::insert(const T& t) {
    
}

template<typename T>
void FineGrainedBST<T>::erase(const T& t) {
    
}

template<typename T>
void FineGrainedBST<T>::find(const T& t) {
    
}

template<typename T>
class LockFreeBST : public BST<T> {
    typename BST<T>::Node* root;
public:
    LockFreeBST();
    virtual void insert(const T& t);
    virtual void erase(const T& t);
    virtual void find(const T& t);
    virtual size_t size();
};

template<typename T>
void LockFreeBST<T>::insert(const T& t) {
    
}

template<typename T>
void LockFreeBST<T>::erase(const T& t) {
    
}

template<typename T>
void LockFreeBST<T>::find(const T& t) {
    
}

#endif
