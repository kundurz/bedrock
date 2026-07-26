#### The Core Concept: DIB
* DIB is the distance from inital bucket.
* DIB = 0: The element is in its ideal, initial hash bucket
* DIB = 3: The element collided and had to move 3 slots away from its ideal bucket

#### How Insertion Works
* When you insert a new key, you hash it to find its home bucket
  * **I like the term "home bucket"**
* If you hit an occupied slot you compare "wealth"
* **The Rule:** If the new key's DIB is greater than the existing key's DIB, the new key is "poorer". The new key evicts the existing key, takes the slot, and the old key is handed off to be re-inserted further down the line.
* By displacing the "rich" elements, the algorithm ensure sthat no single key gets pushed unreasonably far away, smoothing out the cluster lengths across the entire table.