This module provides a means to clone an entire collection from one namespace to another via drush command, optionally deleting the original collection and its members.

The core of this module is adapted from code posted to Google Groups by Alan Stanley in response to the prompt "Changing an object PID" https://groups.google.com/d/msg/islandora/H6zcwb7lnGo/B5x1eELPJ-EJ 

In fact, besides pasting it into a .drush file, the only addition to Alan's code is looping over all collection members, handling compound objects, and some logging. Capacity to handle other compound content types (book, newspaper) could be added fairly easily, but was not necessary for our purposes.

#### Future work

Besides adding support for other content types, it would probably be good to make use of the Batch API.

#### Testing

Check the options available for the command: `drush islandora_change_namespace_collection -h`. Assuming you have a collection ingested with simple and compound objects, for example _latech-cmprt:collectiont_, test this feature from the command line under the web root:

~~~

# clone the collection
drush -u 1 islandora_change_namespace_collection --pid=latech-cmprt:collection --new_pid=latech-cmprt-doppel:collection

# clone an item-  will still be isMemberOf the original collection
drush -u 1 islandora_change_namespace_item --pid=latech-cmprt:7 --new_pid=latech-cmprt-doppel:999

# clone an item, specifying the new parent collection - find it in the specified parent collection 
drush -u 1 islandora_change_namespace_item --pid=latech-cmprt:7 --new_pid=latech-cmprt-doppel:99 --parent=latech-cmprt-doppel:collection

# clone an item into a new parent - note that the original (--pid) is deleted
drush -u 1 islandora_change_namespace_item --pid=latech-cmprt:7 --new_pid=latech-cmprt-doppel:7799 --parent=latech-cmprt-doppel:collection --purge=1

# clone a collection, providing the --parent option - error and exit (currently...)
drush -u 1 islandora_change_namespace_collection --pid=latech-cmprt:collection --new_pid=latech-cmprt-ganger:collection --parent=latech-cmprt-doppel:collection 


# clone the collection with purge option - original will be gone
drush -u 1 islandora_change_namespace_collection --pid=latech-cmprt:collection --new_pid=latech-cmprt-survivor:collection --purge=9

~~~