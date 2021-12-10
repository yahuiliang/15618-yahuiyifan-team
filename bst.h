#ifndef BST_H
#define BST_H

#include <stdio.h>
#include <mutex>
#include <atomic>
#include <cstring>
#include <vector>
#include <memory>
#include <unordered_set>
#include <condition_variable>
#include <climits>

/**
 * The interface for binary search tree definition
 */
template<typename T>
class BST {
protected:
    size_t N; // Thread number for using the bst
    static const size_t R; // Retire list length for each thread
public:
    virtual ~BST() {}
    // Insert element
    virtual bool insert(const T& t)=0;
    // Erase element
    virtual void erase(const T& t)=0;
    // Check whether element exists in the tree
    virtual bool find(const T& t)=0;
    // Tree size
    virtual size_t size()=0;
    // Clear the content of the tree
    virtual void clear()=0;
    // Set the number of thread using the tree
    virtual void set_N(size_t _N) { N = _N; }
    // Register the thread id for the current thread
    virtual void register_thread(size_t tid)=0;
};

template<typename T>
const size_t BST<T>::R = 100;

/**
 * Coarse Grained BST uses the single global mutex to synchronize
 * operations. Concurrent operation is not allowed in this structure.
 * Only one operation can be performed at a time.
 */
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
    // Delete some default contructors and operators which may affect tree structure
    CoarseGrainedBST(const CoarseGrainedBST& other)=delete;
    CoarseGrainedBST& operator=(const CoarseGrainedBST& other)=delete;
    CoarseGrainedBST(const CoarseGrainedBST&& other)=delete;
    CoarseGrainedBST& operator=(const CoarseGrainedBST&& other)=delete;
    virtual bool insert(const T& t);
    virtual void erase(const T& t);
    virtual bool find(const T& t);
    virtual size_t size();
    virtual void clear();
    virtual void register_thread(size_t tid) {};
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

/**
 * The algorithm uses general binary search logic to traverse until
 * the left or right child of the current node is null. Then the new data
 * will be appended as the child of the current node.
 *
 * @param node current node on the traverse path
 * @param element data needs to be inserted
 * @return true if insertion is successful; false otherwise.
 */
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

/**
 * Erase the node whose key matches with the given key.
 *
 * @param parent current node parent
 * @param node current node
 * @param element data needs to be inserted
 */
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

/**
 * Fine Grained BST uses node internal lock to sync node
 * edge modifications
 */
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
    static thread_local size_t thread_id; // Local thread id
    std::vector<std::vector<node_t*>> rlist; // Retire list
    
    /**
     * Atomic variables and locks for GC purpose
     */
    std::atomic<int> rw_count;
    std::mutex mtx;

    node_t* root;
    std::atomic<size_t> _size;
    /**
     * Traverses the tree until it finds the target node.
     * Before the function gets returned, the edge between the 
     * parent and the target will be locked to avoid other threads
     * modifications. Before two nodes get returned, child's key
     * value will be checked against its old value to ensure the child
     * is not slipped away. If the child has been slipped away, the lock
     * on the parent will be released and the traversal will continue.
     *
     * @param node current node
     * @param element key which needs to be found
     * @return target node parent and target node
     */
    std::pair<node_t*, Dir> find_helper(node_t* node, const T& element);
    
    /**
     * Rotate the subtree of node a. Rotation will not reconnect nodes on the
     * original tree directly. Instead, it creates new nodes and insert new nodes
     * into the tree to replace the old one.
     * 
     * @param a root node of subtree
     * @param dir1 child which needs to be rotated away from root
     * @param dir2 child which needs to be rotated toward the root
     * @return a, a->dir1, a->dir2 after the rotation
     */
    std::vector<node_t*> rotation(node_t* a, Dir dir1, Dir dir2);
    void clear(node_t* node);
    
    /**
     * Remove a->dir1 from the tree by reconnecting a->dir1 with a->dir1->dir2.
     * This will isolate old a->dir1.
     *
     * @param a parent node
     * @param dir1 child which needs to be erased
     * @param dir2 child which replaces a->dir1
     */
    void remove(typename FineGrainedBST<T>::node_t* a, Dir dir1, Dir dir2);
    
    /**
     * The function keeps rotating the node which needs to be erased until it 
     * only has one incoming edge and one outgoing edge. By reconnecting the parent
     * node and the child node of the node which needs to removed, the node will be 
     * isolated from the tree.
     *
     * @param f the parent node
     * @param dir f->dir is the node which needs to be removed.
     */
    void deletion_by_rotation(node_t* f, Dir dir);
    
    /**
     * Retires the node. The node will be freed in the future.
     *
     * @param ptr the node pointer
     */
    void retire(node_t* ptr);

    /**
     * Check the retire list length and free nodes if threshold is reached.
     */
    void gc();
public:
    FineGrainedBST();
    virtual ~FineGrainedBST();
    FineGrainedBST(FineGrainedBST& other)=delete;
    FineGrainedBST(FineGrainedBST&& other)=delete;
    FineGrainedBST& operator=(const FineGrainedBST& other)=delete;
    FineGrainedBST& operator=(const FineGrainedBST&& other)=delete;
    /**
     * Find to get the target parent first (find_helper). Then insert the new node to the left or right
     * of the parent node.
     *
     * @param t key which needs to be inserted
     * @return true if successfully inserted; false otherwise.
     */
    virtual bool insert(const T& t);
    
    /**
     * Find to get the target parent first (find_helper). Then use deletion_by_rotation to remove the node.
     *
     * @param t key which needs to be removed
     */
    virtual void erase(const T& t);

    /**
     * Find until the given key is found (find_helper).
     * 
     * @param t target key
     * @return true if found; false otherwise.
     */
    virtual bool find(const T& t);
    virtual size_t size();
    virtual void clear();
    virtual void set_N(size_t _N);
    virtual void register_thread(size_t tid);
};

template<typename T>
thread_local size_t FineGrainedBST<T>::thread_id;

template<typename T>
void FineGrainedBST<T>::set_N(size_t _N) {
    BST<T>::set_N(_N);
    rlist.resize(_N);
}

template<typename T>
void FineGrainedBST<T>::register_thread(size_t tid) {
    thread_id = tid;
}

template<typename T>
void FineGrainedBST<T>::retire(node_t* ptr) {
    rlist[thread_id].push_back(ptr);
}

template<typename T>
void FineGrainedBST<T>::gc() {
    if (rlist[thread_id].size() > BST<T>::R) {
        mtx.lock();
        while (rw_count > 0);
        // Traverse the retire list and clear nodes
        for (node_t* node : rlist[thread_id]) {
            delete node;
        }
        rlist[thread_id].clear();
        mtx.unlock();
    }
}

template<typename T>
FineGrainedBST<T>::FineGrainedBST(): 
    rw_count(0),
    root(new node_t()), _size(0) {}

template<typename T>
FineGrainedBST<T>::~FineGrainedBST() {
    clear();
    delete root;
}

template<typename T>
void FineGrainedBST<T>::clear() {
    clear(root);
    root = new node_t();
    // Traver the retire list and clear nodes
    for (size_t thread_id = 0; thread_id < BST<T>::N; thread_id++) {
        for (node_t* node : rlist[thread_id]) {
            delete node;
        }
        rlist[thread_id].clear();
    }
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
    mtx.lock();
    mtx.unlock();

    rw_count++;

    std::pair<node_t*, Dir> fdir = find_helper(root, t);
    node_t* parent = fdir.first;
    Dir dir = fdir.second;
    node_t* child = parent->children[dir];
    bool inserted = false;
    if (child == nullptr) {
        // Append new child to the parent
        parent->children[dir] = new node_t(t);
        _size++;
        inserted = true;
    }
    parent->mtx.unlock();

    rw_count--;

    return inserted;
}

template<typename T>
std::pair<typename FineGrainedBST<T>::node_t*, typename FineGrainedBST<T>::Dir> FineGrainedBST<T>::find_helper(node_t* node, const T& element) {
    Dir dir;
    if (element < node->val) {
        dir = Dir::Left;
    } else {
        dir = Dir::Right;
    }
    node_t* child = node->children[dir];
    if (child != nullptr && child->val != element) {
        // Not found, keep traversing
        return find_helper(child, element);
    }
    // Found or loop terminate
    node->mtx.lock(); // Lock the parent node
    if (node->color == Color::Blue) {
        // If node has been marked as erased
        node->mtx.unlock();
        // Release lock and keep traversing from back node
        return find_helper(node->back, element);
    } else if (node->children[dir] != child) {
        // The node has been slipped away
        node->mtx.unlock();
        // Keep traversing down
        return find_helper(node, element);
    }
    // Node is found, return it
    return std::pair<node_t*, Dir>(node, dir);
}

template<typename T>
void FineGrainedBST<T>::erase(const T& t) {
    mtx.lock();
    mtx.unlock();

    rw_count++;

    std::pair<node_t*, Dir> fdir = find_helper(root, t);

    node_t* parent = fdir.first;
    Dir dir = fdir.second;
    node_t* child = parent->children[dir];
    if (child == nullptr) {
        parent->mtx.unlock();
    } else {
        child->mtx.lock();
        deletion_by_rotation(parent, dir);
        _size--;
    }

    rw_count--;

    gc();
}

template<typename T>
void FineGrainedBST<T>::deletion_by_rotation(typename FineGrainedBST<T>::node_t* f, Dir dir) {
    node_t* s = f->children[dir];
    if (s->children[Dir::Left] == nullptr) {
        // Erase condition is met, and target node can be removed by reconnecting edges
        remove(f, dir, Dir::Right);
    } else {
        // Rotate target node down
        std::vector<node_t*> fgh = rotation(f, dir, Dir::Left);
        f = fgh[0];
        node_t* g = fgh[1]; // Newly created node
        node_t* h = fgh[2]; // Newly created node, and this is the node we want to remove, and it has been rotated away from f
        if (h->children[Dir::Left] == nullptr) {
            // condition is met, start removing
            deletion_by_rotation(g, Dir::Right);
        } else {
            deletion_by_rotation(g, Dir::Right);
            f->mtx.lock();
            if (g != f->children[dir] || f->color == Color::Blue) {
                // The child has been sliped away or the child has been erased
                f->mtx.unlock();
            } else {
                // Erase complete, and rotate nodes back
                g->mtx.lock();
                std::vector<node_t*> fgh_new = rotation(f, dir, Dir::Right);
                f = fgh_new[0];
                node_t* g_new = fgh_new[1];
                node_t* h_new = fgh_new[2];
                g_new->mtx.unlock();
                h_new->mtx.unlock();
            }
        }
    }
}

template<typename T>
void FineGrainedBST<T>::remove(typename FineGrainedBST<T>::node_t* a, Dir dir1, Dir dir2) {
    node_t* b = a->children[dir1];
    node_t* c = b->children[dir2];
    a->children[dir1] = c;
    b->children[dir2] = c;
    b->back = a;
    b->color = Color::Blue;
    a->mtx.unlock();
    b->mtx.unlock();
    retire(b);
}

template<typename T>
std::vector<typename FineGrainedBST<T>::node_t*> FineGrainedBST<T>::rotation(node_t* a, Dir dir1, Dir dir2) {
    node_t* b = a->children[dir1];
    node_t* c = b->children[dir2];
    node_t* b_new = new node_t();
    node_t* c_new = new node_t();
    
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

    retire(b);
    retire(c);
    return { a, c_new, b_new };
}

template<typename T>
bool FineGrainedBST<T>::find(const T& t) {
    mtx.lock();
    mtx.unlock();

    rw_count++;

    std::pair<node_t*, Dir> fdir = find_helper(root, t);
    node_t* parent = fdir.first;
    Dir dir = fdir.second;
    node_t* child = parent->children[dir];
    bool found = child != nullptr;
    parent->mtx.unlock();

    rw_count--;

    return found;
}

template<typename T>
size_t FineGrainedBST<T>::size() {
    return _size.load();
}

/************************************
 * Macros for dummy nodes as the root
 ************************************/
#define INFINITY_0 INT_MAX - 2
#define INFINITY_1 INT_MAX - 1
#define INFINITY_2 INT_MAX

/**
 * Flag bit is the last bit in the address
 */
static size_t flag_mask = 0x1;
/**
 * Tag bit is the last second bit in the address
 */
static size_t tag_mask = 0x2;
/**
 * Remain bits are addresses
 */
static size_t addr_mask = static_cast<size_t>(0b11);

typedef std::atomic<size_t> atomic_size_t;

template<typename T>
class LockFreeBST : public BST<T> {
    struct node_t {
        T key;
        atomic_size_t left;
        atomic_size_t right;

        node_t(const T& _key) {
            left = 0;
            right = 0;
            key = _key;
        }

        node_t() {
            left = 0;
            right = 0;
        }
    };
    struct seekRecord_t {
        size_t ancestor; // The parent node of the successor node
        size_t successor; // In most cases, it is same as parent. Otherwise, it is the last node which is not being erased
        size_t parent; // The parent node of leaf
        size_t leaf; // The leaf of the tree
    };
    atomic_size_t R_root; // Dummy node
    atomic_size_t S_root; // Dummy node

    atomic_size_t _size;

    static thread_local size_t thread_id; // Local thread id
    std::vector<std::vector<node_t*>> rlist; // Retire list
    
    /**
     * Atomic variables and locks for GC purpose
     */
    std::atomic<int> rw_count;
    std::mutex mtx;

    /**********************************************
     * Helper functions for tag/flag manipulation
     **********************************************/
    size_t set_flag(size_t addr);
    size_t set_tag(size_t addr);
    bool is_flagged(size_t addr);
    bool is_tagged(size_t addr);
    node_t *get_addr(size_t addr);

    void clear(size_t node_addr);

    /**
     * Very similar to the basic BST traverse logic.
     * Instead, it returns the result in seekRecord format.
     *
     * @param key the key which needs to be searched
     * @param seekRecord where the result will be stored to
     */
    void seek(const T& key, struct seekRecord_t *seekRecord);
    
    /**
     * Isolate node by reconnecting ancestor node with the sibling node.
     *
     * @param key the key which needs to be cleaned
     * @param seekRecord nodes which need to be manipulated
     * @return true if cleanup is successful; false otherwise
     */
    bool cleanup(const T& key, const seekRecord_t* seekRecord);

    /**
     * Calling seek to get the leaf location where the new key should be inserted to.
     * Two new nodes will be created in this step. One node is the internal node, and another
     * node is the leaf node which holds the actual key. The internal node value will be 
     * the maximum value between the node at the current leaf location and the given key.
     * Then the old leaf node and the new leaf node will be inserted to left or right of
     * the internal node.
     *
     * @param key the key which needs to be inserted
     * @return true if the key is inserted successfully; false otherwise
     */
    bool insert_helper(const T& key);

    /**
     * The erase operation removes two nodes for each call. One node is the internal node, 
     * and another node is the leaf node which stores the key.Erase operation simply 
     * reconnect the parent of the internal node to the sibling. Therefore, the internal 
     * node and the leaf is isolated from the tree.
     *
     * @param key the key which needs to be erased
     * @return true if the key is inserted successfully; false otherwise
     */
    bool erase_helper(const T& key);

    /**
     * Relies on seek to retrieve record and compare whether the retrieved record
     * is same as the given one.
     *
     * @param key the key which needs to be found
     * @return true if the key is found successfully; false otherwise
     */
    bool find_helper(const T& key);
    
    /**
     * Check the retire list and free nodes if necessary.
     */
    void gc();

    /**
     * Push the node to the retire list.
     *
     * @param ptr the node which needs to be retired
     */
    void retire(node_t* ptr);

    void init();
public:
    LockFreeBST();
    virtual ~LockFreeBST();
    virtual bool insert(const T& t);
    virtual void erase(const T& t);
    virtual bool find(const T& t);
    virtual size_t size();
    virtual void clear();
    virtual void set_N(size_t _N);
    virtual void register_thread(size_t tid);
};

template<typename T>
thread_local size_t LockFreeBST<T>::thread_id;

template<typename T>
void LockFreeBST<T>::set_N(size_t _N) {
    BST<T>::set_N(_N);
    rlist.resize(_N);
}

template<typename T>
void LockFreeBST<T>::register_thread(size_t tid) {
    thread_id = tid;
}

template<typename T>
void LockFreeBST<T>::retire(node_t* ptr) {
    rlist[thread_id].push_back(ptr);
}

template<typename T>
void LockFreeBST<T>::gc() {
    if (rlist[thread_id].size() > BST<T>::R) {
        mtx.lock();
        while (rw_count > 0);
        // Iterate the retire list and free nodes
        for (node_t* node : rlist[thread_id]) {
            delete node;
        }
        rlist[thread_id].clear();
        mtx.unlock();
    }
}

template<typename T>
void LockFreeBST<T>::init() {
    rw_count = 0;
    _size = 0;
    
    /********************
     * Create dummy nodes
     ********************/
    node_t *R_root_n = new node_t(INFINITY_2);
    node_t *S_root_n = new node_t(INFINITY_1);

    R_root = reinterpret_cast<size_t>(R_root_n);
    S_root = reinterpret_cast<size_t>(S_root_n);
    node_t *sentinel_node_0 = new node_t(INFINITY_0);
    node_t *sentinel_node_1 = new node_t(INFINITY_1);
    node_t *sentinel_node_2 = new node_t(INFINITY_2);

    S_root_n->left = reinterpret_cast<size_t>(sentinel_node_0);
    S_root_n->right = reinterpret_cast<size_t>(sentinel_node_1);
    R_root_n->left = reinterpret_cast<size_t>(S_root_n);
    R_root_n->right = reinterpret_cast<size_t>(sentinel_node_2);
}

template<typename T>
LockFreeBST<T>::LockFreeBST() {
    init();
}

template<typename T>
LockFreeBST<T>::~LockFreeBST() {
    clear();
    // Erase dummy nodes
    node_t* R_root_n = get_addr(R_root);
    node_t* S_root_n = get_addr(R_root_n->left);
    node_t* sentinel_node_0 = get_addr(S_root_n->left);
    node_t* sentinel_node_1 = get_addr(S_root_n->right);
    node_t* sentinel_node_2 = get_addr(R_root_n->right);
    delete R_root_n;
    delete S_root_n;
    delete sentinel_node_0;
    delete sentinel_node_1;
    delete sentinel_node_2;
}

template<typename T>
size_t LockFreeBST<T>::set_flag(size_t addr) {
    return addr | flag_mask;
}

template<typename T>
size_t LockFreeBST<T>::set_tag(size_t addr) {
    return addr | tag_mask;
}

template<typename T>
bool LockFreeBST<T>::is_flagged(size_t addr) {
    return (bool)(addr & flag_mask);
}

template<typename T>
bool LockFreeBST<T>::is_tagged(size_t addr) {
    return (bool)((addr & tag_mask) >> 1);
}

template<typename T>
typename LockFreeBST<T>::node_t *LockFreeBST<T>::get_addr(size_t addr) {
    if (addr == 0) {
        return nullptr;
    }
    return (node_t *)(addr & ~addr_mask);
}

template<typename T>
void LockFreeBST<T>::seek(const T& key, struct seekRecord_t *seekRecord) {
    // Init the seek record
    seekRecord->ancestor = R_root;
    seekRecord->successor = S_root;
    seekRecord->parent = S_root;
    size_t S_left = (get_addr(S_root.load())->left).load();
    seekRecord->leaf = reinterpret_cast<size_t>(get_addr(S_left));
    // Init variables used in traversal
    size_t parentField = get_addr(seekRecord->parent)->left;
    size_t currentField = get_addr(seekRecord->leaf)->left;
    node_t *current = get_addr(currentField);
    // Traverse tree
    while (current != nullptr) {
        // Check if the edge from the parent node is tagged
        if (!is_tagged(parentField)) {
            // Advance ancestor and successor
            seekRecord->ancestor = seekRecord->parent;
            seekRecord->successor = seekRecord->leaf;
        }
        // Advance parent and leaf
        seekRecord->parent = seekRecord->leaf;
        seekRecord->leaf = reinterpret_cast<size_t>(current);
        // Update other traversal variables
        parentField = currentField;
        if (key < current->key) {
            currentField = (current->left).load();
        } else {
            currentField = (current->right).load();
        }
        current = get_addr(currentField);
    }
}

template<typename T>
bool LockFreeBST<T>::insert(const T& t) {
    mtx.lock();
    mtx.unlock();

    rw_count++;
    
    bool result = insert_helper(t);
    if (result) {
        _size++;
    }

    rw_count--;

    return result;
}

template<typename T>
bool LockFreeBST<T>::insert_helper(const T& t) {
    // While loop is used for traversing the tree
    while (true) {
        struct seekRecord_t seekRecord;
        seek(t, &seekRecord); // Get the leaf location where the key should be inserted to
        if (get_addr(seekRecord.leaf)->key != t) {
            size_t parent = seekRecord.parent;
            size_t leaf = seekRecord.leaf;
            node_t *parent_n = get_addr(parent);
            node_t *leaf_n = get_addr(leaf);

            // Create internal node and leaf node
            node_t *new_leaf = new node_t(t);
            node_t *newInternal = new node_t();
            if (t < leaf_n->key) {
                newInternal->key = leaf_n->key;
                newInternal->left = reinterpret_cast<size_t>(new_leaf);
                newInternal->right = leaf;
            } else {
                newInternal->key = t;
                newInternal->left = leaf;
                newInternal->right = reinterpret_cast<size_t>(new_leaf);
            }
            size_t old_leaf = reinterpret_cast<size_t>(get_addr(leaf));
            size_t internal = reinterpret_cast<size_t>(newInternal);
            
            atomic_size_t* childAddrPtr;
            bool result;
            // Reconnect internal node, old leaf, and new leaf
            if (t < parent_n->key) {
                childAddrPtr = &parent_n->left;
                result = std::atomic_compare_exchange_weak(&(parent_n->left), &old_leaf, internal);
            } else {
                childAddrPtr = &parent_n->right;
                result = std::atomic_compare_exchange_weak(&(parent_n->right), &old_leaf, internal);
            }
            if (result) {
                // Return if edge modifications are successful
                return true;
            } else {
                // help the conflicting delete operation
                size_t childAddr = *childAddrPtr;
                if (get_addr(childAddr) == leaf_n && (is_flagged(childAddr) || is_tagged(childAddr))) {
                    cleanup(t, &seekRecord);
                }
            }
        } 
        // key existed in the tree
        else {
            return false;
        }
    }
}

enum Mode {
    CLEANUP, INJECTION
};

template<typename T>
void LockFreeBST<T>::erase(const T& key) {
    mtx.lock();
    mtx.unlock();
    
    rw_count++;

    bool result = erase_helper(key);
    if (result) {
        _size--;
    }

    rw_count--;
    gc();
}

template<typename T>
bool LockFreeBST<T>::erase_helper(const T& key) {
    Mode mode = Mode::INJECTION;
    size_t leaf;
    node_t* leaf_n;
    bool done = false;
    while (!done) {
        seekRecord_t seekRecord;
        seek(key, &seekRecord); // Get the parent and the leaf which needs to be erased
        size_t parent = seekRecord.parent;
        node_t* parent_n = get_addr(parent);
        atomic_size_t* childAddrPtr;
        // Get the edge between the parent and the leaf which needs to be erased
        if (key < parent_n->key) {
            childAddrPtr = &(parent_n->left);
        } else {
            childAddrPtr = &(parent_n->right);
        }
        if (mode == Mode::INJECTION) {
            leaf = seekRecord.leaf;
            leaf_n = get_addr(leaf);
            if (leaf_n->key != key) {
                // If key does not exist
                return false;
            }
            // Set flag bit
            size_t old_leaf = reinterpret_cast<size_t>(leaf_n);
            bool result = std::atomic_compare_exchange_weak(
                childAddrPtr, 
                &old_leaf, 
                set_flag(reinterpret_cast<size_t>(leaf_n))
            );
            if (result) {
                mode = Mode::CLEANUP;
                // Cleanup the node
                done = cleanup(key, &seekRecord);
            } else {
                size_t childAddr = *childAddrPtr;
                if (get_addr(childAddr) == leaf_n && (is_flagged(childAddr) || is_tagged(childAddr))) {
                    // If the node has been marked as clean, help to clean the node
                    cleanup(key, &seekRecord);
                }
            }
        } else {
            if (seekRecord.leaf != leaf) {
                // Leaf has been erased
                return false;
            } else {
                // Help to clean
                done = cleanup(key, &seekRecord);
            }
        }
    }
    return done;
}

template<typename T>
bool LockFreeBST<T>::cleanup(const T& key, const seekRecord_t* seekRecord) {
    size_t ancestor = seekRecord->ancestor;
    node_t* ancestor_n = get_addr(ancestor);
    size_t successor = seekRecord->successor;
    node_t* successor_n = get_addr(successor);
    size_t parent = seekRecord->parent;
    node_t* parent_n = get_addr(parent);
    size_t leaf = seekRecord->leaf;
    node_t* leaf_n = get_addr(leaf);
    atomic_size_t* successorAddrPtr;
    if (key < ancestor_n->key) {
        successorAddrPtr = &(ancestor_n->left);
    } else {
        successorAddrPtr = &(ancestor_n->right);
    }
    atomic_size_t* childAddrPtr;
    atomic_size_t* siblingAddrPtr;
    if (key < parent_n->key) {
        childAddrPtr = &(parent_n->left);
        siblingAddrPtr = &(parent_n->right);
    } else {
        childAddrPtr = &(parent_n->right);
        siblingAddrPtr = &(parent_n->left);
    }
    bool flagged = is_flagged(*childAddrPtr);
    if (!flagged) {
        // If the leaf is not marked as clean, then we should not clean this node
        siblingAddrPtr = childAddrPtr;
    }
    
    // Set tag
    *siblingAddrPtr = set_tag(*siblingAddrPtr);
    
    size_t siblingAddr = *siblingAddrPtr;
    flagged = is_flagged(siblingAddr);
    size_t successorExpect = reinterpret_cast<size_t>(successor_n);
    size_t successorNew = siblingAddr;
    if (flagged) {
        // Copy flag bit
        successorNew = set_flag(successorNew);
    }
    // Reconnect the ancester node with the sibling to isolate parent and internal node
    bool result = std::atomic_compare_exchange_weak(
        successorAddrPtr, 
        &successorExpect,
        successorNew & ~tag_mask // Clear the tag bit
    );
    
    if (result) {
        retire(parent_n);
        retire(leaf_n);
    }
    
    return result;
}

template<typename T>
bool LockFreeBST<T>::find(const T& t) {
    mtx.lock();
    mtx.unlock();
    
    rw_count++;

    bool result = find_helper(t);

    rw_count--;

    return result;
}

template<typename T>
bool LockFreeBST<T>::find_helper(const T& t) {
    struct seekRecord_t seekRecord;
    seek(t, &seekRecord);
    if (get_addr(seekRecord.leaf)->key == t) {
        return true;
    }
    return false;
}

template<typename T>
size_t LockFreeBST<T>::size() {
    return _size.load();
}

template<typename T>
void LockFreeBST<T>::clear() {
    clear(R_root.load());
    for (size_t thread_id = 0; thread_id < BST<T>::N; thread_id++) {
        for (node_t* node : rlist[thread_id]) {
            delete node;
        }
        rlist[thread_id].clear();
    }
    init();
}

template<typename T>
void LockFreeBST<T>::clear(size_t node_addr) {
    if (node_addr == 0) {
        return;
    }
    node_t *node = get_addr(node_addr);
    size_t left = node->left.load();
    size_t right = node->right.load();
    clear(left);
    clear(right);
    delete node;
}

#endif
