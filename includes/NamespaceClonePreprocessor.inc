<?php

class NamespaceClonePreprocessor extends IslandoraBatchPreprocessor {

    public function preprocess() {
        $errors = array();
        $p = $this->parameters;
        $source_collection = islandora_object_load($p['source_collection_pid']);
        list($not_used, $members) = islandora_basic_collection_get_member_objects($source_collection, 0, -1);

        $added = array();
        foreach ($members as $member_data) {
            $batch_object = new NamespaceCloneBatchObject($this->connection, $member_data['object']['value'], $this->parameters);
            // Add the instances to the Islandora batch queue.
            $this->addToDatabase($batch_object);
            $added[] = $batch_object;
//            $starting_error_count = count($errors);
//            drush_log(t('Processing @pid', array('@pid' => $member_pid['object']['value'])), 'ok');
        }
        return $added;
    }

    private function addCollectionAndMembersToBatch($collection_pid) {
        if (islandora_object_load($target_collection_pid)) {
            drush_log("Object $target_collection_pid already exists.", 'info');
            $new_namespace = explode(':', $target_collection_pid)[0];
            $new_collection_pid = $target_collection_pid;
        } else {
            $new_collection_pid = clone_object($repository, $source_collection_pid, $target_collection_pid, $errors);
            drush_log(t('Created @pid', array('@pid' => $new_collection_pid)), 'ok');
            $new_namespace = explode(':', $new_collection_pid)[0];
        }
    }

    private function getComplexObjectChildren($object) {
        $classes = array(
            'islandora:compoundCModel' => 'NamespaceCloneCompoundBatchObject'
        );
    }

}
