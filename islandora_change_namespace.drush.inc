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
            'destination_collection' => array(
                'collection to move this to.',
                'value' => 'optional',
            ),
        ),
    );

    $items['islandora_change_namespace_collection'] = array(
        'aliases' => array('icnsptest'),
        'description' => 'Create local collection set to receive Remote Resource content.',
        'drupal dependencies' => array(
            'islandora',
        ),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        'options' => array(
            'source_collection_pid' => array(
                'description' => "object to move to a new namespace.",
                'value' => 'required',
            ),
            'target_collection_pid' => array(
                'description' => 'hostname of the oai endpoint, to be used in constructing request urls.',
                'value' => 'required',
            ),
            'purge' => array(
                'description' => '[ yes | no ] Whether to delete the source_pid object; defaults to no.',
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
 * @param type $source_pid
 * @param type $target_pid
 * @return type
 */
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
    $purge = drush_get_option('purge', FALSE);
    $source_pid = drush_get_option('source_pid');
    $target_pid = drush_get_option('target_pid');
    $dest_coll = drush_get_option('destination_collection', FALSE);
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

function drush_islandora_change_namespace_collection() {
    $purge = drush_get_option('purge', FALSE);
    $source_collection_pid = drush_get_option('source_collection_pid');
    $destination_collection_pid = drush_get_option('target_collection_pid', FALSE);
    module_load_include('inc', 'islandora', 'includes/IslandoraTuque');
    $tuque = new IslandoraTuque();
    $repository = $tuque->repository;
    $destination_collection = clone_object($repository, $source_collection_pid, $destination_collection_pid);
    drush_log(t('Created @pid', array('@pid' => $destination_collection)), 'ok');

    $source_collection = islandora_object_load($source_collection_pid);
    module_load_include('module', 'islandora_basic_collection', 'islandora_basic_collection');
    list($not_used, $members) = islandora_basic_collection_get_member_objects($source_collection);
    $new_namespace = explode(':', $destination_collection)[0];
    foreach ($members as $member) {
        $new_member_pid = clone_object($repository, $member['object']['value'], $new_namespace);
        drush_log(t('Created @pid', array('@pid' => $new_member_pid)), 'ok');
        islandora_change_namespace_migrate_item($new_member_pid, $destination_collection);
        $member_object = islandora_object_load($member['object']['value']);
        if (in_array('islandora:compoundCModel', $member_object->models)) {
            module_load_include('module', 'islandora_compound_object', 'islandora_compound_object');
            module_load_include('inc', 'islandora_compound_object', 'includes/manage.form');
            $constituents = islandora_compound_object_get_parts($member_object->id);
            $new_member = islandora_object_load($new_member_pid);

            foreach ($constituents as $constituent) {
                $new_constituent_pid = clone_object($repository, $constituent, $new_namespace);
                $new_constituent = islandora_object_load($new_constituent_pid);
                islandora_compound_object_add_parent(array($new_constituent), array($new_member));
                islandora_compound_object_remove_parent(array($new_constituent), array($member_object));
            }
        }
        if ($purge) {
            $repository->purgeObject($member['object']['value']);
            drush_log(t('Purged @pid', array('@pid' => $member_object->id)), 'ok');
        }
    }

    if ($purge) {
        $repository->purgeObject($source_collection_pid);
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
            '@object' => $object->id,
            '@collection' => $new_collection->label));
        drush_log($message, 'ok');
    }
}
