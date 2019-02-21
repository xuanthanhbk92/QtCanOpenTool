#ifndef SPARSEARRAY_H
#define SPARSEARRAY_H

#include <vector>

template<typename Key_T, typename Value_T> class SparseArray {
public:
    struct Entry_T {
        Key_T key; Value_T value;

        explicit Entry_T (const Key_T key, const Value_T & value) : key (key), value (value) { }
    };

    explicit SparseArray (void) { }

    inline bool insert (const Key_T key, const Value_T & value) {
        bool ret = false;
        if (!contains (key)) {
            if (_entries.empty () || _entries.back ().key < key) {
                _entries.push_back (Entry_T (key, value));
                ret = true;
            }
            else {
                for (typename std::vector<Entry_T>::iterator it = _entries.begin (); it < _entries.end (); ++it) {
                    if ((* it).key >= key) {
                        _entries.insert (it, Entry_T (key, value));
                        ret = true;
                        break;
                    }
                }
            }
        }
        return ret;
    }

    inline Value_T value (const Key_T key, const Value_T & fallback) const {
        const int tmp = indexOf (key);
        return (tmp >= 0 ? _entries [tmp].value : fallback);
    }

    inline bool contains (const Key_T key) const {
        return (indexOf (key) >= 0);
    }

    inline int count (void) const {
        return _entries.size ();
    }

    typedef typename std::vector<Entry_T>::const_iterator iterator;

    inline iterator begin (void) const { return _entries.cbegin (); }
    inline iterator end   (void) const { return _entries.cend   (); }

private:
    std::vector<Entry_T> _entries;

    inline int indexOf (const Key_T key) const {
        int ret = -1;
        int left = 0;
        int right = (int (_entries.size ()) -1);
        while (left <= right) {
            const int middle = (left + (right - left) / 2);
            if (_entries [middle].key == key) { // Check if key is present at middle
                ret = middle;
                break;
            }
            else if (_entries [middle].key > key) {// If key is smaller, ignore right half
                right = (middle -1);
            }
            else if (_entries [middle].key < key) { // If key greater, ignore left half
                left = (middle +1);
            }
            else { }
        }
        return ret;
    }
};

#endif // SPARSEARRAY_H
