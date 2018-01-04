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

    $items['islandora_change_namespace_qa_compare_pids'] = array(
        'description' => 'Compare the datastreams of two pids.',
        'drupal dependencies' => array(
            'islandora',
        ),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        'options' => array(
            'orig' => array(
                'description' => "original object which was the src of a clone.",
                'value' => 'required',
            ),
            'clone' => array(
                'description' => 'pid of clone.',
                'value' => 'required',
            ),
        ),
    );

    $items['islandora_change_namespace_qa_compare_cpd_sequence'] = array(
        'description' => 'Compare the sequence of two compound objects.',
        'drupal dependencies' => array(
            'islandora',
        ),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        'options' => array(
            'orig' => array(
                'description' => "original object which was the src of a clone.",
                'value' => 'required',
            ),
            'clone' => array(
                'description' => 'pid of clone.',
                'value' => 'required',
            ),
            'dry-run' => array(
                'description' => 'Set to any value to prevent reordering.',
                'value' => 'optional',
            ),
        ),
    );

    $items['islandora_change_namespace_qa_compare_cpd_sequence_collection'] = array(
        'description' => 'Compare the sequence of compound objects between origin collection and clone collection.',
        'drupal dependencies' => array(
            'islandora',
        ),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        'options' => array(
            'origin' => array(
                'description' => "original collection which was the src of a clone.",
                'value' => 'required',
            ),
            'clone' => array(
                'description' => 'pid of clone collection.',
                'value' => 'required',
            ),
            'dry-run' => array(
                'description' => 'Set to any value to prevent reordering.',
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
    $items['islandora_change_namespace_collection_qa'] = array(
        'aliases' => array('icnsptest'),
        'description' => 'Compare origin collection to clone collection.',
        'drupal dependencies' => array(
            'islandora',
        ),
        'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        'options' => array(
            'origin' => array(
                'description' => "origin pid.",
                'value' => 'required',
            ),
            'clone' => array(
                'description' => 'clone pid.',
                'value' => 'required',
            ),
            'icns_verbose' => array(
                'description' => 'verbose messaging.',
                'value' => 'optional',
            ),
        ),
    );
    return $items;
}

function drush_islandora_change_namespace_collection_qa() {
    $origin_pid = drush_get_option('origin');
    $clone_pid  = drush_get_option('clone');
    module_load_include('inc', 'islandora_change_namespace', 'includes/qa');
    islandora_change_namespace_collection_qa($origin_pid, $clone_pid);
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

function drush_islandora_change_namespace_qa_compare_pids() {
    $comp = new IslandoraChangeNamespacePidComparison(drush_get_option('orig'), drush_get_option('clone'));
    $comp->compareDatastreams();
    $comp->print_discrepancies();
}

class IslandoraChangeNamespacePidComparison {

    public $original, $clone;
    public $discrepancies = array();
    public $ignored = array('RELS-EXT', 'DC');

    public function __construct($original_pid, $clone_pid) {
        $this->original = islandora_object_load($original_pid);
        $this->clone = islandora_object_load($clone_pid);
    }

    public function print_discrepancies() {
        foreach ($this->discrepancies as $dsid) {
            printf("[ %s ===>>> %s ]      %s\n", $this->original->id, $this->clone->id, $dsid);
        }
        if (empty($this->discrepancies)) {
            printf("\nNo discrepancies found between %s and %s.\nNB: Datastreams [%s] were not checked.\n\n", $this->original->id, $this->clone, implode('|', $this->ignored));
        }
    }

    public function compareDatastreams() {

        foreach ($this->original as $dsid => $ds) {
            if (in_array($dsid, $this->ignored)) {
                continue;
            }

            if (md5($ds->content) != md5($this->clone[$dsid]->content)) {
                $this->discrepancies[] = $dsid;
            }
        }
    }

}


class IslandoraChangeNamespaceCpdObject {

    public $pid, $object, $sequence;

    public function __construct($pid) {
        $this->pid = $pid;
        $this->object = islandora_object_load($this->pid);
    }

    public function members() {
        module_load_include('module', 'islandora_compound_object', 'islandora_compound_object');

        return islandora_compound_object_get_parts($this->pid, TRUE);
    }

    public function common_format() {
        $common = array();
        foreach ($this->members() as $pid => $data) {
            $hash = md5(islandora_object_load($pid)['MODS']->content);
            $common[$hash] = $data;
        }
        return $common;
    }

}

function drush_islandora_change_namespace_qa_compare_cpd_sequence() {
    $o = new IslandoraChangeNamespaceCpdObject(drush_get_option('orig'));
    $c = new IslandoraChangeNamespaceCpdObject(drush_get_option('clone'));

    update_cpd_sequence($o, $c);
}

function update_cpd_sequence(IslandoraChangeNamespaceCpdObject $origin, IslandoraChangeNamespaceCpdObject $clone) {
    $discrepancies = 0;
    $clone_sequence_update = array();
    $clone_sequence = $clone->common_format();
    drush_log(t("Comparing cpd origin @origin to clone @clone", array('@origin' => $origin->pid, '@clone' => $clone->pid)), 'ok');
    foreach ($origin->common_format() as $hash => $data) {

        if ($data['seq'] != $clone_sequence[$hash]['seq']) {
            $discrepancies++;

            drush_log(t("- @pid: @old =>>> @new", array(
                '@pid' => $clone_sequence[$hash]['pid'],
                '@old' => $clone_sequence[$hash]['seq'],
                '@new' => $data['seq'],
                    )), 'ok');
            $clone_sequence_update[$hash] = $clone_sequence[$hash];
            $clone_sequence_update[$hash]['seq'] = $data['seq'];
        }
    }
    if ($discrepancies > 0) {
        if (!drush_get_option('dry-run', FALSE)) {
            $update_tn = FALSE;
            foreach ($clone_sequence_update as $hash => $data) {
                drush_log(t("- - Reordering pid @pid =>>> @new", array(
                    '@pid' => $clone_sequence_update[$hash]['pid'],
                    '@new' => $data['seq'],
                        )), 'ok');
                $clone_sequence_update[$hash]['seq'] = $data['seq'];
                $escaped_pid = str_replace(':', '_', $clone->pid);
                $child_object = islandora_object_load($data['pid']);
                $child_object->relationships->remove(ISLANDORA_RELS_EXT_URI, "isSequenceNumberOf$escaped_pid");
                $child_object->relationships->add(ISLANDORA_RELS_EXT_URI, "isSequenceNumberOf$escaped_pid", $data['seq'], RELS_TYPE_PLAIN_LITERAL);
                if ($data['seq'] == '1') {
                    $update_tn = TRUE;
                }
            }
            if ($update_tn) {
                module_load_include('inc', 'islandora_compound_object', 'includes/manage.form');
                drush_log(t("- Setting TN for @pid", array('@pid' => $clone->pid,)), 'ok');
                islandora_compound_object_change_thumbnail($clone->pid);
            }
        }
    } else {
        drush_log(t("- Ok"), 'ok');
    }
}

function drush_islandora_change_namespace_qa_compare_cpd_sequence_collection() {
    $original_collection_pid = drush_get_option('origin');
    $clone_collection_pid = drush_get_option('clone');
    $results_original = get_compounds_for_collection($original_collection_pid);
    $results_clone = get_compounds_for_collection($clone_collection_pid);
    $map = get_map_origin_clone($results_original, $results_clone);
    foreach ($map as $origin => $clone) {
        update_cpd_sequence(new IslandoraChangeNamespaceCpdObject($origin), new IslandoraChangeNamespaceCpdObject($clone));
    }
}

function get_map_origin_clone($results_origin, $results_clone) {
    $map = array();
    $tmp = array();
    $fingerprint = function($pid) {
        $obj = islandora_object_load($pid);
        return md5($obj['MODS']->content);
    };
    foreach ($results_origin as $data) {
        $pid = $data['object']['value'];
        $tmp[$fingerprint($pid)] = $pid;
    }
    foreach ($results_clone as $data) {
        $pid = $data['object']['value'];
        $map[$tmp[$fingerprint($pid)]] = $pid;
    }
    return $map;
}

function get_compounds_for_collection($collection_pid) {
    module_load_include('inc', 'islandora', 'includes/IslandoraTuque');
    $tuque = new IslandoraTuque();
    $repository = $tuque->repository;
    $query = <<<EOF
SELECT ?object
FROM <#ri>
WHERE {
?object <fedora-rels-ext:isMemberOfCollection> <info:fedora/%s> .
?object <fedora-model:hasModel> <info:fedora/islandora:compoundCModel>
}
EOF;
    return $repository->ri->sparqlQuery(sprintf($query, $collection_pid), 'unlimited');
}
