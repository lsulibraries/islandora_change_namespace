<?php

class NamespaceCloneBatchObject extends IslandoraBatchObject {

    protected $srcPid;
    protected $preprocessorParameters;
    protected $params;

    /**
     * Constructor.
     */
    public function __construct(IslandoraTuque $connection, $srcPid, $batch_parameters) {
        $nextPid = $connection->repository->getNextIdentifier($batch_parameters['new_namespace']);
        parent::__construct($nextPid, $connection->repository);
        $this->srcPid = $srcPid;
        $this->params = $batch_parameters;
    }

    public function batchProcess() {
        module_load_include('inc', 'islandora_change_namespace', 'includes/utilities');
        $purge = FALSE;
        $errors = array();
        $source_object = islandora_object_load($this->srcPid);
        var_dump($source_object->models); die();
//        $new_object = $repository->constructObject($new_pid);
        $this->label = $source_object->label;
        foreach ($source_object as $dsid => $content) {
            if (!$source_object[$dsid]->content) {
                $msg = "$source_pid: No content found for DSID $dsid in object $source_pid while trying to clone into namespace $new_pid.";
                drush_log($msg, 'warning');
                $errors[] = $msg;
            } else {
                $this->ingestDatastream($content);
            }
        }
        $this->relationships->changeObjectID($this->id);
//        $ingested_object = islandora_add_object($this);
//        $newItemPid = clone_object($this->repository, $this->srcPid, $this->params['new_namespace'], $errors);
//        drush_log(t(' - Created @pid', array('@pid' => $newItemPid)), 'ok');
//
        islandora_change_namespace_migrate_item($this, $this->params['new_collection_pid']);
        $srcItem = islandora_object_load($this->srcPid);
        if (in_array('islandora:compoundCModel', $srcItem->models)) {
            drush_log(t(' --- Processing compound constituents:'), 'ok');
            module_load_include('module', 'islandora_compound_object', 'islandora_compound_object');
            module_load_include('inc', 'islandora_compound_object', 'includes/manage.form');

            $constituents = islandora_compound_object_get_parts($srcItem->id);
            $compound_error_count = array();
            foreach ($constituents as $constituent) {
                $new_constituent_pid = clone_object($this->repository, $constituent, $p['new_namespace'], $errors);

                $new_constituent = islandora_object_load($new_constituent_pid);
                islandora_compound_object_add_parent(array($new_constituent), array($this));
                islandora_compound_object_remove_parent(array($new_constituent), array($srcItem));
                drush_log(t(' ---- Created constituent @pid', array('@pid' => $new_constituent->id)), 'ok');

                if ($purge) {
                    if (count($errors) == $compound_error_count) {
                        $this->connection->repository->purgeObject($constituent);
                        drush_log(t(' ---- Purged constituent @pid', array('@pid' => $constituent)), 'ok');
                    } else {
                        drush_log(t(' ---- Not purging constituent due to errors @pid', array('@pid' => $constituent)), 'ok');
                    }
                }
            }
        }
//        if ($purge) {
//            if (count($errors) == $starting_error_count) {
//                $this->repository->purgeObject($this->srcPid);
//                drush_log(t(' ----- Purged @pid', array('@pid' => $srcItem->id)), 'ok');
//            } else {
//                drush_log(t(' ----- Not purging @pid', array('@pid' => $srcItem->id)), 'warning');
//            }
//        }
//        if ($purge) {
//            if (empty($errors)) {
//                $this->repository->purgeObject($pid);
//                drush_log(t('- Purged collection @pid', array('@pid' => $pid)), 'ok');
//            } else {
//                drush_log(t("Not purging collection @pid due to errors.", array('@pid' => $pid)), "warning");
//            }
//        }
        return ISLANDORA_BATCH_STATE__DONE;
    }

    public function addRelationships() {
        
    }

    public function getResources() {
        
    }

}
