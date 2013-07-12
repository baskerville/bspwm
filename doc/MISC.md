# Mathematical background

The main data structure is a full binary tree.

A binary tree is *full* if each of its node has either two or zero children.

If a node has two children it is an internal node, otherwise a leaf.

Fundamental theorem:
Let I be the number of internal nodes and L the number of leaves, then:
    L = I + 1

(It can be proved by induction on the number of internal nodes.)

This means that when we add a leaf to the tree (when a window is created), we must also add one internal node.
