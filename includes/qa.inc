<?php

use Psr\Log\LogLevel;

function islandora_change_namespace_collection_qa($origin_pid, $clone_pid) {
  $logfile = sprintf("/tmp/drush-%s__%s_%s.log", str_replace(':', '-', $origin_pid), str_replace(':', '-', $clone_pid), time());
  $collection_pair = new IslandoraChangeNamespaceCollectionObjectPair($origin_pid, $clone_pid, $logfile);
  $results = $collection_pair->compare();
  $report_header = "\n\n\n=======\nResults\n=======\n\n";
  if (empty($results)) {
    $ok = "No discrepancies!";
    drush_log($ok, LogLevel::NOTICE);
    file_put_contents($logfile, sprintf("%s%s", $report_header, $ok), FILE_APPEND);
  }
  else {
    drush_log("Discrepancies found: ", LogLevel::WARNING);
    natsort($results);
    foreach ($results as $result) {
      drush_log($result, LogLevel::ERROR);
    }
    $report = sprintf("%s%s", $report_header, implode("\n", $results));
    file_put_contents($logfile, $report, FILE_APPEND);
  }
  drush_log(sprintf("done. Find results in file %s", $logfile), LogLevel::NOTICE);
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
      $log_msg = sprintf("   [ %s ==>> %s ] %s", $this->origin->id, $this->clone->id, $msg);
      $this->log[] = $log_msg;
      drush_log($log_msg, $drush_level);
      file_put_contents($this->logfile, sprintf("[ %-6s ]      %s%s", $drush_level, $log_msg, "\n"), FILE_APPEND);
    }
  }

  protected function compareDatastreams() {
    $origin_ds_count = count($this->origin);
    $clone_ds_count = count($this->clone);
    $missing = $results = array();

    if ($origin_ds_count != $clone_ds_count) {
      $this->log(sprintf("Datastream count mismatch: %s ==>> %s", $origin_ds_count, $clone_ds_count), LogLevel::ERROR);
      foreach ($this->origin as $dsid => $data) {
        if (!$this->clone[$dsid]) {
          $result = sprintf("%s missing from clone.", $dsid);
          $this->log($result, LogLevel::ERROR);
          $missing[] = $dsid;
          $results[] = $result;
        }
      }
      foreach ($this->clone as $dsid => $data) {
        if (!$this->origin[$dsid]) {
          $result = sprintf("%s missing from origin.", $dsid);
          $this->log($result, LogLevel::ERROR);
          $missing[] = $dsid;
          $results[] = $result;
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
        $results[] = sprintf("[ %s ==>> %s ] discrepancy in %s", $this->origin->id, $this->clone->id, $dsid);
        $this->log(sprintf("%s differs.", $dsid), LogLevel::ERROR);
      }
      else {
        $this->log(sprintf("%s matches.", $dsid), LogLevel::DEBUG);
      }
    }
    return $results;
  }

  public function compare() {
    $results = $this->compareDatastreams();

    if (in_array('islandora:compoundCModel', $this->origin->models)) {
      $compound = new IslandoraChangeNamespaceCompoundObjectPair($this->origin->id, $this->clone->id, $this->logfile);
      $results = array_merge($results, $compound->compare());
    }
    return $results;
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
    $results = $this->compareDatastreams();
    return array_merge($results, $this->compareChildren());
  }

  protected function compareChildren() {
    list($origin_children, $clone_children) = $this->getChildren();
    $map = $this->getMap($origin_children, $clone_children);
    $this->log("Comparing child objects ...", LogLevel::NOTICE);
    $results = array();
    foreach ($map as $pair) {
      if (!array_key_exists('o', $pair)) {
        $result = sprintf("%s missing mate (or MODS mismatch) in origin", $pair['c']);
        $results[] = $result;
        $this->log($result, LogLevel::ERROR);
      }
      elseif (!array_key_exists('c', $pair)) {
        $result = sprintf("%s missing mate (or MODS mismatch) in clone", $pair['o']);
        $results[] = $result;
        $this->log($result, LogLevel::ERROR);
      }
      else {
        $comp = new IslandoraChangeNamespaceObjectPair($pair['o'], $pair['c'], $this->logfile);
        $results = array_merge($results, $comp->compare());
      }
    }
    return $results;
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
