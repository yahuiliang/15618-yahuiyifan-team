#ifndef BST_H
#define BST_H

template<typename T>
class BST {
public:
    struct Node {
        Node* left;
        Node* right;
        T val;
    };
    virtual void insert(const T& t)=0;
    virtual void erase(const T& t)=0;
    virtual void find(const T& t)=0;
};

template<typename T>
class CoarseGrainedBST : public BST<T> {
    typename BST<T>::Node* root;
public:
    CoarseGrainedBST();
    void insert(const T& t);
    void erase(const T& t);
    void find(const T& t);
};

template<typename T>
CoarseGrainedBST<T>::CoarseGrainedBST() {
    
}

template<typename T>
void CoarseGrainedBST<T>::insert(const T& t) {
    
}

template<typename T>
void CoarseGrainedBST<T>::erase(const T& t) {
    
}

template<typename T>
void CoarseGrainedBST<T>::find(const T& t) {
    
}

template<typename T>
class FineGrainedBST : public BST<T> {
    typename BST<T>::Node* root;
public:
    FineGrainedBST();
    void insert(const T& t);
    void erase(const T& t);
    void find(const T& t);
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
    void insert(const T& t);
    void erase(const T& t);
    void find(const T& t);
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
