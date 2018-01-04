<?php

use Psr\Log\LogLevel;

function islandora_change_namespace_collection_qa($origin_pid, $clone_pid) {
  $logfile = sprintf("/tmp/drush-%s__%s_%s.log", str_replace(':', '-', $origin_pid), str_replace(':', '-', $clone_pid), time());
  $collection_pair = new IslandoraChangeNamespaceCollectionObjectPair($origin_pid, $clone_pid, $logfile);
  $collection_pair->compare();
}

class IslandoraChangeNamespaceObjectPair {

  public $origin, $clone, $log = array(), $logfile, $verbose;
  protected $ignored = array('DC', 'RELS-EXT');

  public function __construct($origin_pid, $clone_pid, $logfile) {
    $this->logfile = $logfile;
    $this->verbose = drush_get_option('icns_verbose', FALSE);
    $this->origin = islandora_object_load($origin_pid);
    $this->clone  = islandora_object_load($clone_pid);

    if (!$this->origin || !$this->clone) {
      $missing = $this->origin ? $clone_pid : $origin_pid;
      $this->log(sprintf("Error creating pair; %s missing.", $missing), LogLevel::ERROR);
    }
  }

  public function log($msg, $drush_level) {
    $debug = drush_get_context()['cli']['debug'] && $drush_level == LogLevel::DEBUG ? LogLevel::DEBUG : NULL;
    $verbose_levels = array(LogLevel::NOTICE, $debug);
    if ($drush_level == LogLevel::ERROR || ($this->verbose && in_array($drush_level, $verbose_levels))) {
      $log_msg = sprintf("[ %s ]      [ %s ==>> %s ] %s", $drush_level, $this->origin->id, $this->clone->id, $msg);
      $this->log[] = $log_msg;
      drush_log($log_msg, $drush_level);
      file_put_contents($this->logfile, $log_msg . "\n", FILE_APPEND);
    }
  }

  protected function compareDatastreams() {
    $origin_ds_count = count($this->origin);
    $clone_ds_count = count($this->clone);
    $missing = array();
    if ($origin_ds_count != $clone_ds_count) {
      $this->log(sprintf("Datastream count mismatch: %s ==>> %s", $origin_ds_count, $clone_ds_count), LogLevel::ERROR);
      foreach ($this->origin as $dsid => $data) {
        if (!$this->clone[$dsid]) {
          $this->log(sprintf("%s missing from clone.", $dsid), LogLevel::ERROR);
          $missing[] = $dsid;
        }
      }
      foreach ($this->clone as $dsid => $data) {
        if (!$this->origin[$dsid]) {
          $this->log(sprintf("%s missing from origin.", $dsid), LogLevel::ERROR);
          $missing[] = $dsid;
        }
      }
    }
    else {
      $this->log(sprintf("Datastream count ok: %s ==>> %s", $origin_ds_count, $clone_ds_count), LogLevel::DEBUG);
    }
    $this->log("Comparing datastreams ...", LogLevel::NOTICE);
    foreach ($this->origin as $dsid => $data) {
      if (in_array($dsid, $this->ignored) || in_array($dsid, $missing)) {
        $this->log(sprintf("ignoring comparison for %s....", $dsid), LogLevel::DEBUG);
        continue;
      }
      if (md5($this->origin[$dsid]->content) != md5($this->clone[$dsid]->content)) {
        $this->log(sprintf("%s differs.", $dsid), LogLevel::ERROR);
      }
      else {
        $this->log(sprintf("%s matches.", $dsid), LogLevel::DEBUG);
      }
    }
  }

  public function compare() {
    $this->compareDatastreams();
    if (in_array('islandora:compoundCModel', $this->origin->models)) {
      $compound = new IslandoraChangeNamespaceCompoundObjectPair($this->origin->id, $this->clone->id, $this->logfile);
      $compound->compare();
    }
  }

}

abstract class IslandoraChangeNamespaceContainerObjectPair extends IslandoraChangeNamespaceObjectPair {

  protected $map, $origin_children, $clone_children;

  protected function getMap($origin_children, $clone_children) {
    $this->map = array();
    foreach ($origin_children as $child) {
      $this->addToMap($child, 'o');
    }
    foreach ($clone_children as $child) {
      $this->addToMap($child, 'c');
    }
    return $this->map;
  }

  private function addToMap($pid, $type) {
    $child = islandora_object_load($pid);
    $hash = md5($child['MODS']->content);
    if (!array_key_exists($hash, $this->map)) {
      $this->map[$hash] = [];
    }
    $this->map[$hash][$type] = $child->id;
  }

  public function compare() {
    $this->compareDatastreams();
    $this->compareChildren();
  }

  protected function compareChildren() {
    list($origin_children, $clone_children) = $this->getChildren();
    $map = $this->getMap($origin_children, $clone_children);
    $this->log("Comparing child objects ...", LogLevel::NOTICE);
    foreach ($map as $pair) {
      if (!array_key_exists('o', $pair)) {
        $this->log(sprintf("%s missing mate in origin", $pair['c']), LogLevel::ERROR);
      }
      elseif (!array_key_exists('c', $pair)) {
        $this->log(sprintf("%s missing mate in clone", $pair['o']), LogLevel::ERROR);
      }
      else {
        $comp = new IslandoraChangeNamespaceObjectPair($pair['o'], $pair['c'], $this->logfile);
        $comp->compare();
      }
    }
  }

  abstract function getChildren();
}

class IslandoraChangeNamespaceCollectionObjectPair extends IslandoraChangeNamespaceContainerObjectPair {

  public function getChildren() {
    list($not_used, $origin_children_raw) = islandora_basic_collection_get_member_objects($this->origin, 0, -1);
    list($no_usage, $clone_children_raw) = islandora_basic_collection_get_member_objects($this->clone, 0, -1);
    $extract_pids = function($a) {
      return $a['object']['value'];
    };
    $origin_children = array_map($extract_pids, $origin_children_raw);
    $clone_children = array_map($extract_pids, $clone_children_raw);
    return array($origin_children, $clone_children);
  }

}

class IslandoraChangeNamespaceCompoundObjectPair extends IslandoraChangeNamespaceContainerObjectPair {

  public function getChildren() {
    module_load_include('module', 'islandora_compound_object', 'islandora_compound_object');
    $origin_constituent_parts = islandora_compound_object_get_parts($this->origin->id, TRUE);
    $clone_constituent_parts = islandora_compound_object_get_parts($this->clone->id, TRUE);

    $origin_children = array_keys($origin_constituent_parts);
    $clone_children = array_keys($clone_constituent_parts);

    return array($origin_children, $clone_children);
  }
}
