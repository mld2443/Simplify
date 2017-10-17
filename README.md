# Simplify
Automated mesh simplification using quantitative edge collapsing. This project was completed [for an assignment](http://faculty.cs.tamu.edu/schaefer/teaching/645_Fall2015/assignments/hw4.html) in fall of 2015. As such, it is rough, and probably contains numerous poor conventions such as lack of comments.

<img width="416" src="https://user-images.githubusercontent.com/5340992/31679042-92bd1564-b335-11e7-99d1-2a83dd870ec0.png"><img width="416" src="https://user-images.githubusercontent.com/5340992/31679082-aeb895d6-b335-11e7-966c-57b3b8c89c20.png">

## Problem Summary
We want to be able to take a manifold as input, and a number of faces for the desired output. The program will then simplify the manifold until the number of faces is equal to or less than the desired number, or it reaches a point where it cannot simplify the geometry anymore.

## Algorithm
Upon receiving an input manifold surface, we convert it into a halfedge mesh, and calculate a Quadratic Error Function (QEF, described below) for each edge, inserting the quantified edges into a priority queue which sorts on the error of their prospective collapse. We then pull the edges from the queue to collapse them. A single collapse removes two faces from the mesh.

### Edges
Once an edge is collapsed, all the neighboring edges that were once part of the previous vertices are now "dirty" which means that their QEFs no longer reflect their true value. This is handled by setting all neighbors as "dirty after a collapse; when we pull an edge out of the queue that's dirty, we simply update its QEF and reinsert it into the queue.

Edges can also be uncollapsible. This might be the case if a collapse of the edge would make a "flap" where two faces are mirror opposites. How these are handled used to be tedious, involving a second queue. This has since been simplified, as described below.

<img width="416" src="https://user-images.githubusercontent.com/5340992/31677010-0768b62c-b32f-11e7-8947-ff1c531d3eb0.png"><img width="416" src="https://user-images.githubusercontent.com/5340992/31677058-364d6f1e-b32f-11e7-9300-f5bd6de31ac1.png">

## Analysis
The runtime of this program is (to me at least) incredibly fast. Models with a million or more faces take only a few seconds to simplify to a few thousand faces.

The QEF I use is the summation of the squares of all the distances from the vertex to all its neighbors. The minimizer of a vertex is the average of all its neighbors. This QEF has several pros: 
* It is simple to implement, I do not need to store much for each vertex, since the summation is essentially 5 numbers in 3D space, (two scalars and a point), regardless of vertex valence.
* Combining QEFs of this type is to only add them together.
* Combined QEFs keep the information about the error of the vertices they used to represent, meaning that a combined QEF of two vertices is not the same as the QEF of the vertex that would lie at its minimizer.
* Combining the QEFs of two vertices on the same plane will yield a computable result, meaning I don't have to implement a pseudoinverse function.

The biggest drawback of this QEF is that it favors edges with vertices who simply have the closer neighbors, instead of edges which contribute least to the overall shape. This means that on shapes with varying degrees of high and low detail areas, this QEF is far from mathematically optimal and will produce many edges of near uniform length. This results in a stylized "polygonal" look, which is neat yet unfaithful to the original shape.

### What I've learned
I ended up updating the part of my algorithm that handles the uncollapsible edges. Edges now have an "unsafe" flag, and if I compute that they are uncollapsible, I set it to true and remove it from the priority queue. When I do end up collapsing an edge, I then set all neighboring edges to "safe" again and reinsert it into the priority queue. What's curious about this is it's nearly identical to how I handle the "dirty" edges, and I just assumed that I needed to handle them differently.

<img width="416" src="https://user-images.githubusercontent.com/5340992/31679120-cc9e29b2-b335-11e7-9703-b5be1d32d628.png"><img width="416" src="https://user-images.githubusercontent.com/5340992/31679149-e258af48-b335-11e7-82a5-f7ad47a046bd.png">

## Future Work
Some time in the future, I intend to write a version of this which can use a more descriptive QEF, (likely involving a pseudoinverse). I would also take the opportunity to rewrite the whole program in a different environment.
