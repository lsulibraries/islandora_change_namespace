<?php

class NamespaceClonePreprocessor extends IslandoraBatchPreprocessor {

    public $errors = array();

    public function preprocess() {
        $this->addCollectionAndMembersToBatch();
    }

    private function addCollectionAndMembersToBatch() {

        $source_collection = islandora_object_load($this->parameters['source_collection_pid']);
        list($not_used, $members) = islandora_basic_collection_get_member_objects($source_collection, 0, -1);

        $added = array();
        foreach ($members as $member_data) {
            $pid = $member_data['object']['value'];
            $batch_object = new NamespaceCloneBatchObject($this->connection, $pid, $this->parameters);
            // Add the instances to the Islandora batch queue.
            $this->addToDatabase($batch_object);
            $added[] = $batch_object;
            drush_log(t(' -- Pre-Processed member @pid:', array('@pid' => $pid)), 'ok');

            $object = islandora_object_load($pid);
            if (in_array('islandora:compoundCModel', $object->models)) {
                $added += $this->addCompoundConstituents($object, $batch_object);
            }
        }
        return $added;
    }

    private function addCompoundConstituents($cpdSrcParentObject, $newParentObject) {
        drush_log(t(' --- Pre-Processing compound constituents for @cpdParent:', array('@cpdParent' => $cpdSrcParentObject->id)), 'ok');
        module_load_include('module', 'islandora_compound_object', 'islandora_compound_object');

        $constituents = islandora_compound_object_get_parts($cpdSrcParentObject->id);
        $compound_error_count = array();
        foreach ($constituents as $constituent_pid) {
            $this->parameters['old_parent_pid'] = $cpdSrcParentObject->id;
            $this->parameters['new_parent_pid'] = $newParentObject->id;
            $batch_object = new NamespaceCloneBatchObjectCpdConstituent($this->connection, $constituent_pid, $this->parameters);
            $this->addToDatabase($batch_object);
            $added[] = $batch_object;
            drush_log(t(' ---- Pre-Processed constituent @constituent:', array('@constituent' => $constituent_pid)), 'ok');
        }
        return $added;
    }

}
