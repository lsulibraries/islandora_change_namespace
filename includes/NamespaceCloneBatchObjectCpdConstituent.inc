<?php

class NamespaceCloneBatchObjectCpdConstituent extends NamespaceCloneBatchObject {

    protected $srcPid;
    protected $preprocessorParameters;
    protected $params;

    public function batchProcess() {
        $this->cloneDatastreams();
        $this->relationships->changeObjectID($this->id);
        module_load_include('inc', 'islandora_compound_object', 'includes/manage.form');

        islandora_compound_object_add_parent(array($this), array($this->params['new_parent_pid']));
        islandora_compound_object_remove_parent(array($this), array($this->params['old_parent_pid']));
        drush_log(t(' ---- Created constituent @pid', array('@pid' => $this->id)), 'ok');
        return ISLANDORA_BATCH_STATE__DONE;
    }

    public function addRelationships() {
        
    }

    public function getResources() {
        
    }

}
