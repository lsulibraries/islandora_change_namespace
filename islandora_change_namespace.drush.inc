<?php

/**
 * Implements hook_drush_command().
 */
function islandora_change_namespace_drush_command() {
  $items = array();

  $items['islandora_change_namespace_item'] = array(
    'aliases' => array('icnsptest'),
    'description' => 'Clone an object into another namespace, optionally deleting the original.',
    'drupal dependencies' => array(
      'islandora',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    'options' => array(
      'pid' => array(
        'description' => "object to move to a new namespace.",
        'value' => 'required',
      ),
      'new_pid' => array(
        'description' => 'hostname of the oai endpoint, to be used in constructing request urls.',
        'value' => 'required',
      ),
      'purge' => array(
        'description' => 'Delete the source object. Defaults to FALSE; supplying any value sets TRUE.',
        'value' => 'optional',
      ),
      'parent' => array(
        'description' => 'Collection to clone an item into.',
        'value' => 'optional',
      ),
    ),
  );

  $items['islandora_change_namespace_collection'] = array(
    'aliases' => array('icnsptest'),
    'description' => 'Clone a collection into another namespace, optionally deleting the original.',
    'drupal dependencies' => array(
      'islandora',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    'options' => array(
      'pid' => array(
        'description' => "object to move to a new namespace.",
        'value' => 'required',
      ),
      'new_pid' => array(
        'description' => 'hostname of the oai endpoint, to be used in constructing request urls.',
        'value' => 'required',
      ),
      'purge' => array(
        'description' => 'Delete the source collection (and members). Defaults to FALSE; supplying any value sets TRUE.',
        'value' => 'optional',
      ),
    ),
  );
  return $items;
}

/**
 * Clones an object from one namespace to another.
 *
 * Gleefully adapted from code posted by Alan Stanley to Google Groups:
 * https://groups.google.com/d/msg/islandora/H6zcwb7lnGo/B5x1eELPJ-EJ
 *
 * @param type $repository
 * @param string $source_pid
 * @param string $new_pid
 * @return string The newly-minted pid of the cloned object.
 */
function clone_object($repository, $source_pid, $new_pid) {
  $source_object = islandora_object_load($source_pid);
  $new_object = $repository->constructObject($new_pid);
  $new_object->label = $source_object->label;
  foreach ($source_object as $dsid => $content) {
    $new_object->ingestDatastream($content);
  }
  $new_object->relationships->changeObjectID($new_object->id);
  $ingested_object = islandora_add_object($new_object);
  return $ingested_object->id;
}

/**
 * Initialize common vars.
 * 
 * @return array 
 */
function get_drush_options() {
  $purge = drush_get_option('purge', FALSE);
  $pid = drush_get_option('pid');
  $new_pid = drush_get_option('new_pid');
  $parent = drush_get_option('parent', FALSE);
  if (islandora_object_load($new_pid)) {
    drush_set_error("Object $new_pid already exists.");
    drush_die();
  }
  return array($purge, $pid, $new_pid, $parent);
}

function drush_islandora_change_namespace_item() {
  list($purge, $source_pid, $target_pid, $parent) = get_drush_options();
  module_load_include('inc', 'islandora', 'includes/IslandoraTuque');
  $tuque = new IslandoraTuque();
  $repository = $tuque->repository;
  $new_pid = clone_object($repository, $source_pid, $target_pid);
  if ($parent) {
    islandora_change_namespace_migrate_item($new_pid, $parent);
  }
  drupal_set_message(t('Created @pid', array('@pid' => $new_pid)));
  if ($purge) {
    $repository->purgeObject($source_pid);
  }
}

function drush_islandora_change_namespace_collection() {
  list($purge, $pid, $new_pid, $parent) = get_drush_options();
  module_load_include('module', 'islandora_basic_collection', 'islandora_basic_collection');
  module_load_include('inc', 'islandora', 'includes/IslandoraTuque');
  $tuque = new IslandoraTuque();
  $repository = $tuque->repository;

  $new_collection_pid = clone_object($repository, $pid, $new_pid);
  drush_log(t('Created @pid', array('@pid' => $new_collection_pid)), 'ok');
  $new_namespace = explode(':', $new_collection_pid)[0];

  $source_collection = islandora_object_load($pid);
  list($not_used, $members) = islandora_basic_collection_get_member_objects($source_collection, 0, -1);

  foreach ($members as $member_pid) {
    drush_log(t('Processing @pid', array('@pid' => $member_pid['object']['value'])), 'ok');

    $new_member_pid = clone_object($repository, $member_pid['object']['value'], $new_namespace);
    drush_log(t(' - Created @pid', array('@pid' => $new_member_pid)), 'ok');

    islandora_change_namespace_migrate_item($new_member_pid, $new_collection_pid);

    $member = islandora_object_load($member_pid['object']['value']);
    if (in_array('islandora:compoundCModel', $member->models)) {
      drush_log(t(' --- Processing compound constituents:'), 'ok');
      module_load_include('module', 'islandora_compound_object', 'islandora_compound_object');
      module_load_include('inc', 'islandora_compound_object', 'includes/manage.form');

      $constituents = islandora_compound_object_get_parts($member->id);
      $new_member = islandora_object_load($new_member_pid);

      foreach ($constituents as $constituent) {
        $new_constituent_pid = clone_object($repository, $constituent, $new_namespace);

        $new_constituent = islandora_object_load($new_constituent_pid);
        islandora_compound_object_add_parent(array($new_constituent), array($new_member));
        islandora_compound_object_remove_parent(array($new_constituent), array($member));
        drush_log(t(' ---- Created constituent @pid', array('@pid' => $new_constituent->id)), 'ok');

        if ($purge) {
          $repository->purgeObject($constituent);
          drush_log(t(' ---- Purged constituent @pid', array('@pid' => $constituent)), 'ok');
        }
      }
    }
    if ($purge) {
      $repository->purgeObject($member_pid['object']['value']);
      drush_log(t(' ----- Purged @pid', array('@pid' => $member->id)), 'ok');
    }
  }

  if ($purge) {
    $repository->purgeObject($pid);
    drush_log(t('- Purged collection @pid', array('@pid' => $pid)), 'ok');
  }
}

/**
 * Adaapted from collection solution pack.
 * 
 * @param string $pid
 * @param string $new_collection_pid
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
    $message = t(' -- The object @object has been added to @collection', array(
      '@object' => $object->id,
      '@collection' => $new_collection->id));
    drush_log($message, 'ok');
  }
}
