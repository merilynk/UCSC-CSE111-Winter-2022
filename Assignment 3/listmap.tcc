// $Id: listmap.tcc,v 1.15 2019-10-30 12:44:53-07 - - $

#include "listmap.h"
#include "debug.h"

//
/////////////////////////////////////////////////////////////////
// Operations on listmap.
/////////////////////////////////////////////////////////////////
//

//
// listmap::~listmap()
//
template <typename key_t, typename mapped_t, class less_t>
listmap<key_t,mapped_t,less_t>::~listmap() {
   DEBUGF ('l', reinterpret_cast<const void*> (this));
   while (not empty()) erase(begin());
}

//
// iterator listmap::insert (const value_type&)
//
template <typename key_t, typename mapped_t, class less_t>
typename listmap<key_t,mapped_t,less_t>::iterator
listmap<key_t,mapped_t,less_t>::insert (const value_type& pair) {
   
   listmap<key_t, mapped_t, less_t>::iterator it = begin();

   if (begin() == end()) {  // check if list is empty
      // create a node for the list
      node* new_node = new node(anchor(), anchor(), pair);
      anchor()->next = new_node;
      anchor()->prev = new_node;
      ++it;
      return it;
   } 
   else {
      it = find(pair.first);  // find the key value pair
      if (it != end()) {  // key was found
         it->second = pair.second;  // replace the value
         return it;
      }
      else {  // key was not found
         it = begin();
         // find the place the the key should be inserted into
         while (it != end() && less(it->first, pair.first)) {  
            ++it;
         }
         if (it == begin()) {
            // insert the node at the frong
            node* new_node = new node(anchor()->next, anchor(), pair);
            anchor()->next->prev = new_node;
            anchor()->next = new_node;
            --it;
         }
         else if (it == end()) {
            // insert the node at the end
            node* new_node = new node(anchor(), anchor()->prev, pair);
            anchor()->prev->next = new_node;
            anchor()->prev = new_node;
            --it;
         }
         else {
            // inserting node somewhere in the middle
            node* current_node = it.where;
            node* new_node = 
            new node(current_node, current_node->prev, pair);
            current_node->prev->next = new_node;
            current_node->prev = new_node;
            --it;
         }
         return it;
      }
   }
}

//
// listmap::find(const key_type&)
//
template <typename key_t, typename mapped_t, class less_t>
typename listmap<key_t,mapped_t,less_t>::iterator
listmap<key_t,mapped_t,less_t>::find (const key_type& that) {
   DEBUGF ('l', that);
   listmap<key_t,mapped_t,less_t>::iterator it = begin();
   while (it != end() && it->first != that) ++it;
   return it;
}

//
// iterator listmap::erase (iterator position)
//
template <typename key_t, typename mapped_t, class less_t>
typename listmap<key_t,mapped_t,less_t>::iterator
listmap<key_t,mapped_t,less_t>::erase (iterator position) {
   DEBUGF ('l', &*position);
   node* current_node = position.where;
   current_node->prev->next = current_node->next;
   current_node->next->prev = current_node->prev;
   position = nullptr;
   return position;
}
