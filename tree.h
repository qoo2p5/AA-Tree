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
            if (this->right->level > 0) {
                return this->right->go_left();
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
            if (this->left->level > 0) {
                return this->left->go_right();
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
            return this->node->value;
        }

        const ValueType* operator->() const {
            return &(this->node->value);
        }

        bool operator==(const iterator& other) const {
            return this->node == other.node && this->is_end == other.is_end;
        }

        bool operator!=(const iterator& other) const {
            return this->node != other.node || this->is_end != other.is_end;
        }

        iterator operator++() {
            const Node* next = this->node->get_next();
            if (next->level == 0) {
                this->is_end = true;
            } else {
                this->node = next;
            }
            return *this;
        }

        const iterator operator++(int) {
            iterator tmp = *this;
            this->operator++();
            return tmp;
        }

        iterator operator--() {
            if (this->is_end) {
                this->is_end = false;
            } else {
                this->node = this->node->get_previous();
            }
            return *this;
        }

        const iterator operator--(int) {
            iterator tmp = *this;
            this->operator--();
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
        this->initialize();
    }

    template<class InputIterator>
    Set(InputIterator first, InputIterator last) {
        this->initialize();
        while (first != last) {
            this->insert(*first++);
        }
    }

    Set(std::initializer_list<ValueType> init) {
        this->initialize();
        for (const ValueType& value : init) {
            this->insert(value);
        }
    }

    Set(const Set<ValueType>& other) {
        this->initialize();
        for (const ValueType& value : other) {
            this->insert(value);
        }
    }

    Set& operator=(const Set<ValueType>& other) {
        if (this != &other) {
            this->clear();
            for (const ValueType& value : other) {
                this->insert(value);
            }
        }
        return *this;
    }

    ~Set() {
        this->clear();
        delete this->bottom;
    }

    size_t size() const {
        return this->node_count;
    }

    bool empty() const {
        return this->node_count == 0;
    }

    void insert(const ValueType& value) {
        bool is_inserted;
        this->root = internal_insert(this->root, is_inserted, value);
        if (is_inserted) {
            node_count++;
        }
    }

    void erase(const ValueType& value) {
        to_erase = nullptr;
        bool is_erased;
        this->root = internal_erase(this->root, is_erased, value);
        this->root->parent = this->bottom;
        if (is_erased) {
            node_count--;
        }
    }

    iterator lower_bound(const ValueType& value) const {
        Node* result = internal_find(value);
        if (result == this->bottom) {
            return this->end();
        } else {
            return iterator(result);
        }
    }

    iterator find(const ValueType& value) const {
        Node* v = internal_find(value);
        if (v == this->bottom || value < v->value) {
            return this->end();
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
        this->recursive_delete(root);
        this->root = bottom;
        this->node_count = 0;
    }

 private:
    size_t node_count;
    Node* bottom;
    Node* root;

    void initialize() {
        this->node_count = 0;
        this->bottom = new Node();
        this->bottom->level = 0;
        this->bottom->left = this->bottom;
        this->bottom->right = this->bottom;
        this->bottom->parent = this->bottom;
        this->root = this->bottom;
    }

    Node* make_node(const ValueType& value) {
        Node* result = new Node(value);
        result->level = 1;
        result->left = this->bottom;
        result->right = this->bottom;
        result->parent = this->bottom;
        return result;
    }

    void recursive_delete(Node* node) {
        if (node == this->bottom) {
            return;
        }
        this->recursive_delete(node->left);
        this->recursive_delete(node->right);
        delete node;
    }

    Node* rotate_left(Node* v) {
        Node* parent = v->parent;
        Node* new_v = v->right;
        v->right = new_v->left;
        if (v->right != this->bottom) {
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
        if (v->left != this->bottom) {
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
        if (node == this->bottom) {
            return node;
        }
        if (node->level == node->left->level) {
            return this->rotate_right(node);
        }
        return node;
    }

    Node* split(Node* node) {
        if (node == this->bottom) {
            return node;
        }
        if (node->level == node->right->level && node->level == node->right->right->level) {
            node->right->level++;
            return this->rotate_left(node);
        }
        return node;
    }

    Node* internal_find(const ValueType& value) const {
        Node* cur = this->root;
        Node* last = this->bottom;
        while (cur != this->bottom) {
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

    Node* internal_insert(Node* node, bool& is_inserted, const ValueType& value) {
        if (node == this->bottom) {
            is_inserted = true;
            return this->make_node(value);
        }

        if (value < node->value) {
            node->left = this->internal_insert(node->left, is_inserted, value);
            node->left->parent = node;
        } else if (node->value < value) {
            node->right = this->internal_insert(node->right, is_inserted, value);
            node->right->parent = node;
        } else {
            is_inserted = false;
        }

        if (is_inserted) {
            node = skew(node);
            node = split(node);
        }
        return node;
    }

    Node* to_erase;

    Node* internal_erase(Node* node, bool& is_erased, const ValueType& value) {
        static Node* last;

        is_erased = false;
        if (node == this->bottom) {
            return node;
        }
        last = node;

        if (value < node->value) {
            node->left = internal_erase(node->left, is_erased, value);
            if (node->left != this->bottom) {
                node->left->parent = node;
            }
        } else {
            to_erase = node;
            node->right = internal_erase(node->right, is_erased, value);
            if (node->right != this->bottom) {
                node->right->parent = node;
            }
        }

        if (node == last && to_erase && !(to_erase->value < value) && !(value < to_erase->value)) {
            is_erased = true;
            to_erase->value = last->value;
            if (last == to_erase) {
                node = last->left;
            } else {
                node = last->right;
            }
            delete last;
        }

        if (node->left->level + 1 < node->level || node->right->level + 1 < node->level) {
            node->level--;
            if (node->right->level > node->level) {
                node->right->level--;
            }
            node = this->skew(node);
            this->skew(node->right);
            this->skew(node->right->right);
            node = this->split(node);
            this->split(node->right);
        }

        return node;
    }
};

