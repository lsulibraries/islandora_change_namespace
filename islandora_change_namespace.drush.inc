<?php

/**
 * Implements hook_drush_command().
 */
function islandora_change_namespace_drush_command() {
    $items = array();

    $items['islandora_change_namespace_pid'] = array(
        'aliases' => array('icnsptest'),
        'description' => 'Create local collection set to receive Remote Resource content.',
        'drupal dependencies' => array(
            'islandora',
        ),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        'options' => array(
            'source_pid' => array(
                'description' => "object to move to a new namespace.",
                'value' => 'required',
            ),
            'target_pid' => array(
                'description' => 'hostname of the oai endpoint, to be used in constructing request urls.',
                'value' => 'required',
            ),
            'purge' => array(
                'description' => '[ yes | no ] Whether to delete the source_pid object; defaults to no.',
                'value' => 'optional',
            ),
            'destination_collection' => 'collection to move this to.',
            'value' => 'optional',
        ),
    );
    return $items;
}

function clone_object($repository, $source_pid, $target_pid) {
    $source_object = islandora_object_load($source_pid);
    $new_object = $repository->constructObject($target_pid);
    $new_object->label = $source_object->label;
    foreach ($source_object as $dsid => $content) {
        $new_object->ingestDatastream($content);
    }
    $new_object->relationships->changeObjectID($new_object->id);
    $ingested_object = islandora_add_object($new_object);
    $new_pid = $ingested_object->id;
    return $new_pid;
}

function drush_islandora_change_namespace_pid() {
//    $collection = drush_get_option()
    $purge = drush_get_option('purge', FALSE);
    $source_pid = drush_get_option('source_pid');
    $target_pid = drush_get_option('target_pid');
    $dest_coll  = drush_get_option('destination_collection', FALSE);
    module_load_include('inc', 'islandora', 'includes/IslandoraTuque');
    $tuque = new IslandoraTuque();
    $repository = $tuque->repository;
    $new_pid = clone_object($repository, $source_pid, $target_pid);
    if ($dest_coll) {
        islandora_change_namespace_migrate_item($new_pid, $dest_coll);
    }
    drupal_set_message(t('Created @pid', array('@pid' => $new_pid)));
    if ($purge) {
        $repository->purgeObject($source_pid);
    }
}

/**
 * Adaapted from collection solution pack.
 * 
 * Processes the form and migrates the objects.
 *
 */
function islandora_change_namespace_migrate_item($pid, $new_collection_pid) {
  module_load_include('inc', 'islandora_basic_collection', 'includes/utilities');
  $object = islandora_object_load($pid);
  $new_collection = islandora_object_load($new_collection_pid);
  $current_parents = islandora_basic_collection_get_parent_pids($object);
  if ($object && $new_collection) {
    foreach ($current_parents as $parents) {
      $parent = islandora_object_load($parents);
      islandora_basic_collection_remove_from_collection($object, $parent);
    }
    islandora_basic_collection_add_to_collection($object, $new_collection);
    $message = t('The object @object has been added to @collection', array(
                 '@object' => $object->label,
                 '@collection' => $new_collection->label));
    drupal_set_message($message);

  }

}