<?php

class NamespaceCloneBatchObjectCpdConstituent extends NamespaceCloneBatchObject {

    protected $srcPid;
    protected $preprocessorParameters;
    protected $params;

    public function batchProcess() {
        $this->cloneDatastreams();
        $this->relationships->changeObjectID($this->id);
        module_load_include('inc', 'islandora_compound_object', 'includes/manage.form');

        // This is probably not needed as we remove and re-create this 
        // relationship further on when we set the seq num explicitly.
        islandora_compound_object_add_parent(array($this), array($this->params['new_parent_pid']));
        islandora_compound_object_remove_parent(array($this), array($this->params['old_parent_pid']));

        // Set the sequence number explicitly.
        $escaped_pid = str_replace(':', '_', $this->params['new_parent_pid']);
        $seq_number = $this->params['sequence_number'];
        $this->relationships->remove(ISLANDORA_RELS_EXT_URI, "isSequenceNumberOf$escaped_pid");
        $this->relationships->add(ISLANDORA_RELS_EXT_URI, "isSequenceNumberOf$escaped_pid", $seq_number, RELS_TYPE_PLAIN_LITERAL);

        drush_log(t(' ---- Created constituent @pid', array('@pid' => $this->id)), 'ok');
        return ISLANDORA_BATCH_STATE__DONE;
    }

    public function addRelationships() {
        
    }

    public function getResources() {
        
    }

}
