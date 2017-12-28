<?php


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
function clone_object($repository, $source_pid, $new_pid, &$errors) {
  $source_object = islandora_object_load($source_pid);
  $new_object = $repository->constructObject($new_pid);
  $new_object->label = $source_object->label;
  foreach ($source_object as $dsid => $content) {
    if (!$source_object[$dsid]->content) {
      $msg = "$source_pid: No content found for DSID $dsid in object $source_pid while trying to clone into namespace $new_pid.";
      drush_log($msg, 'warning');
      $errors[] = $msg;
    }
    else {
      $new_object->ingestDatastream($content);
    }
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
  if (!islandora_object_load($pid)) {
    drush_set_error("Source pid $pid does not exist!");
    drush_die();
  }

  return array($purge, $pid, $new_pid, $parent);
}

function display_errors($errors) {
  if (!empty($errors)) {
    $err_str = implode(',', $errors);
    drush_log(t("Errors occurred during processing: \n @errors", array('@errors' => $err_str)), 'warning');
    $err_file = "/tmp/drush-ns-clone-errors-" . time();
    file_put_contents($err_file, $err_str);
    drush_log(t("Errors can be found in @error_file", array('@error_file' => $err_file)), 'warning');
  }
  else {
    drush_log("No errors encountered during processing.", 'ok');
  }
}

/**
 * Adaapted from collection solution pack.
 * 
 * @param string $pid
 * @param string $new_collection_pid
 */
function islandora_change_namespace_migrate_item($object, $new_collection_pid) {
    module_load_include('inc', 'islandora_basic_collection', 'includes/utilities');
//    $object = islandora_object_load($pid);
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
