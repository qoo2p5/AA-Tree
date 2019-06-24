// Copyright (c) 2019 Daniil Nikolenko
// Implementation of AA tree, http://user.it.uu.se/~arnea/ps/simp.pdf

#pragma once

#include <cstddef>
#include <initializer_list>
#include <memory>

template<class ValueType>
class Set {
 private:
    struct Node {
        ValueType value;
        Node* left;
        Node* right;
        Node* parent;
        size_t level;

        Node() {}

        explicit Node(const ValueType& value) : value(value) {}

        Node(const ValueType& value, Node* left, Node* right, Node* parent, size_t level) :
                value(value), left(left), right(right), parent(parent), level(level) {}

        const Node* go_left() const {
            const Node* result = this;
            while (result->left->level > 0) {
                result = result->left;
            }
            return result;
        }

        const Node* go_right() const {
            const Node* result = this;
            while (result->right->level > 0) {
                result = result->right;
            }
            return result;
        }

        const Node* get_next() const {
            if (right->level > 0) {
                return right->go_left();
            }
            const Node* cur = this;
            while (cur->level > 0) {
                if (cur->parent->left == cur) {
                    return cur->parent;
                }
                cur = cur->parent;
            }
            return cur;
        }

        const Node* get_previous() const {
            if (left->level > 0) {
                return left->go_right();
            }
            const Node* cur = this;
            while (cur->level > 0) {
                if (cur->parent->right == cur) {
                    return cur->parent;
                }
                cur = cur->parent;
            }
            return cur;
        }
    };

 public:
    class iterator {
     public:
        iterator() : node(nullptr), is_end(false) {}

        const ValueType& operator*() const {
            return node->value;
        }

        const ValueType* operator->() const {
            return &(node->value);
        }

        bool operator==(const iterator& other) const {
            return node == other.node && is_end == other.is_end;
        }

        bool operator!=(const iterator& other) const {
            return node != other.node || is_end != other.is_end;
        }

        iterator operator++() {
            const Node* next = node->get_next();
            if (next->level == 0) {
                is_end = true;
            } else {
                node = next;
            }
            return *this;
        }

        const iterator operator++(int) {
            iterator tmp = *this;
            operator++();
            return tmp;
        }

        iterator operator--() {
            if (is_end) {
                is_end = false;
            } else {
                node = node->get_previous();
            }
            return *this;
        }

        const iterator operator--(int) {
            iterator tmp = *this;
            operator--();
            return tmp;
        }

        friend iterator Set<ValueType>::begin() const;
        friend iterator Set<ValueType>::end() const;
        friend iterator Set<ValueType>::lower_bound(const ValueType&) const;
        friend iterator Set<ValueType>::find(const ValueType&) const;

     private:
        const Node* node;
        bool is_end;

        explicit iterator(const Node* node, bool is_end = false) {
            this->is_end = (node->level == 0) ? true : is_end;
            this->node = node;
        }
    };

    Set() {
        initialize();
    }

    template<class InputIterator>
    Set(InputIterator first, InputIterator last) {
        initialize();
        while (first != last) {
            insert(*first++);
        }
    }

    Set(std::initializer_list<ValueType> init) {
        initialize();
        for (const ValueType& value : init) {
            insert(value);
        }
    }

    Set(const Set<ValueType>& other) {
        initialize();
        for (const ValueType& value : other) {
            insert(value);
        }
    }

    Set& operator=(const Set<ValueType>& other) {
        if (this != &other) {
            clear();
            for (const ValueType& value : other) {
                insert(value);
            }
        }
        return *this;
    }

    ~Set() {
        clear();
        delete bottom;
    }

    size_t size() const {
        return node_count;
    }

    bool empty() const {
        return node_count == 0;
    }

    void insert(const ValueType& value) {
        bool is_inserted;
        root = internal_insert(root, &is_inserted, value);
        if (is_inserted) {
            node_count++;
        }
    }

    void erase(const ValueType& value) {
        bool is_erased;
        root = internal_erase(root, &is_erased, value);
        root->parent = bottom;
        if (is_erased) {
            node_count--;
        }
    }

    iterator lower_bound(const ValueType& value) const {
        Node* result = internal_find(value);
        return (result == bottom) ? end() : iterator(result);
    }

    iterator find(const ValueType& value) const {
        Node* v = internal_find(value);
        if (v == bottom || value < v->value) {
            return end();
        } else {
            return iterator(v);
        }
    }

    iterator begin() const {
        return iterator(root->go_left());
    }

    iterator end() const {
        return iterator(root->go_right(), true);
    }

    void clear() {
        recursive_delete(root);
        root = bottom;
        node_count = 0;
    }

 private:
    size_t node_count;
    Node* bottom;
    Node* root;

    void initialize() {
        node_count = 0;
        bottom = new Node();
        bottom->level = 0;
        bottom->left = bottom;
        bottom->right = bottom;
        bottom->parent = bottom;
        root = bottom;
    }

    Node* make_node(const ValueType& value) {
        return new Node(value, bottom, bottom, bottom, 1);
    }

    void recursive_delete(Node* node) {
        if (node == bottom) {
            return;
        }
        recursive_delete(node->left);
        recursive_delete(node->right);
        delete node;
    }

    Node* rotate_left(Node* v) {
        Node* parent = v->parent;
        Node* new_v = v->right;
        v->right = new_v->left;
        if (v->right != bottom) {
            v->right->parent = v;
        }
        new_v->left = v;
        v->parent = new_v;
        new_v->parent = parent;
        if (parent->left == v) {
            parent->left = new_v;
        } else if (parent->right == v) {
            parent->right = new_v;
        }
        return new_v;
    }

    Node* rotate_right(Node* v) {
        Node* parent = v->parent;
        Node* new_v = v->left;
        v->left = new_v->right;
        if (v->left != bottom) {
            v->left->parent = v;
        }
        new_v->right = v;
        v->parent = new_v;
        new_v->parent = parent;
        if (parent->left == v) {
            parent->left = new_v;
        } else if (parent->right == v) {
            parent->right = new_v;
        }
        return new_v;
    }

    Node* skew(Node* node) {
        if (node == bottom) {
            return node;
        }
        if (node->level == node->left->level) {
            return rotate_right(node);
        }
        return node;
    }

    Node* split(Node* node) {
        if (node == bottom) {
            return node;
        }
        if (node->level == node->right->level && node->level == node->right->right->level) {
            node->right->level++;
            return rotate_left(node);
        }
        return node;
    }

    Node* internal_find(const ValueType& value) const {
        Node* cur = root;
        Node* last = bottom;
        while (cur != bottom) {
            if (value < cur->value) {
                last = cur;
                cur = cur->left;
            } else if (cur->value < value) {
                cur = cur->right;
            } else {
                last = cur;
                break;
            }
        }
        return last;
    }

    Node* internal_insert(Node* node, bool* is_inserted, const ValueType& value) {
        if (node == bottom) {
            *is_inserted = true;
            return make_node(value);
        }

        if (value < node->value) {
            node->left = internal_insert(node->left, is_inserted, value);
            node->left->parent = node;
        } else if (node->value < value) {
            node->right = internal_insert(node->right, is_inserted, value);
            node->right->parent = node;
        } else {
            *is_inserted = false;
        }

        if (*is_inserted) {
            node = skew(node);
            node = split(node);
        }
        return node;
    }

    Node* internal_erase(Node* node, bool* is_erased, const ValueType& value) {
        Node* to_erase = nullptr;
        Node* last = nullptr;
        return internal_erase(node, is_erased, value, &to_erase, &last);
    }

    Node* internal_erase(Node* node, bool* is_erased, const ValueType& value, Node** to_erase, Node** last) {
        *is_erased = false;
        if (node == bottom) {
            return node;
        }
        *last = node;

        if (value < node->value) {
            node->left = internal_erase(node->left, is_erased, value, to_erase, last);
            if (node->left != bottom) {
                node->left->parent = node;
            }
        } else {
            *to_erase = node;
            node->right = internal_erase(node->right, is_erased, value, to_erase, last);
            if (node->right != bottom) {
                node->right->parent = node;
            }
        }

        if (node == *last && *to_erase && !((*to_erase)->value < value) && !(value < (*to_erase)->value)) {
            *is_erased = true;
            (*to_erase)->value = (*last)->value;
            if (*last == *to_erase) {
                node = (*last)->left;
            } else {
                node = (*last)->right;
            }
            delete *last;
        }

        if (node->left->level + 1 < node->level || node->right->level + 1 < node->level) {
            node->level--;
            if (node->right->level > node->level) {
                node->right->level--;
            }
            node = skew(node);
            skew(node->right);
            skew(node->right->right);
            node = split(node);
            split(node->right);
        }

        return node;
    }
};
