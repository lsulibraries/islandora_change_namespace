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

function drush_islandora_change_namespace_item() {
    module_load_include('inc', 'islandora_change_namespace', 'includes/utilities');
    module_load_include('inc', 'islandora', 'includes/IslandoraTuque');
    list($purge, $source_pid, $target_pid, $parent) = get_drush_options();
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
    module_load_include('inc', 'islandora_change_namespace', 'includes/utilities');
    $errors = array();
    list($purge, $source_collection_pid, $target_collection_pid, $parent) = get_drush_options();
    module_load_include('module', 'islandora_basic_collection', 'islandora_basic_collection');
    module_load_include('inc', 'islandora', 'includes/IslandoraTuque');
    $tuque = new IslandoraTuque();
    $repository = $tuque->repository;

    if (islandora_object_load($target_collection_pid)) {
        drush_log("Object $target_collection_pid already exists.", 'info');
        $new_namespace = explode(':', $target_collection_pid)[0];
        $new_collection_pid = $target_collection_pid;
    } else {
        $new_collection_pid = clone_object($repository, $source_collection_pid, $target_collection_pid, $errors);
        drush_log(t('Created @pid', array('@pid' => $new_collection_pid)), 'ok');
        $new_namespace = explode(':', $new_collection_pid)[0];
    }

    $preprocessor_params = array(
        'source_collection_pid' => $source_collection_pid,
        'new_namespace' => $new_namespace,
        'new_collection_pid' => $new_collection_pid,
    );
    $preprocessor = new NamespaceClonePreprocessor($tuque, $preprocessor_params);
    $preprocessed = islandora_batch_handle_preprocessor($preprocessor);
    drush_log(t("SetId: @s", array('@s' => $preprocessor->getSetId())), "ok");
}
