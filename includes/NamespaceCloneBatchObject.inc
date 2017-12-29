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

    protected function cloneDatastreams() {
        module_load_include('inc', 'islandora_change_namespace', 'includes/utilities');
        $source_object = islandora_object_load($this->srcPid);
        $this->label = $source_object->label;
        foreach ($source_object as $dsid => $content) {
            if (!$source_object[$dsid]->content) {
                $msg = "$source_pid: No content found for DSID $dsid in object $source_pid while trying to clone into namespace $new_pid.";
                drush_log($msg, 'warning');
            } else {
                $this->ingestDatastream($content);
            }
        }
    }

    public function batchProcess() {
        $this->cloneDatastreams();
        $this->relationships->changeObjectID($this->id);
        islandora_change_namespace_migrate_item($this, $this->params['new_collection_pid']);

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
