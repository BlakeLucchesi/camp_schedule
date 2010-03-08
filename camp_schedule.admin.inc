<?php

/**
 * Schedule settings.
 */
function camp_schedule_settings_form() {
  $types = node_get_types();
  foreach ($types as $type) {
    $options[$type->type] = $type->name;
  }
  
  $form['camp_schedule_node_types'] = array(
    '#title' => 'Node Types',
    '#type' => 'checkboxes', 
    '#options' => $options,
    '#default_value' => _camp_schedule_node_types(),
  );           
  return system_settings_form($form);
}

/**
 * Manage schedule form.
 */
function camp_schedule_manage_form(&$form_state) {
  $form = array();
  $form['#content_extra_fields'] = $type['extra'];
  $form['#pre_render'][] = 'content_alter_extra_weights';  
  module_load_include('inc', 'content', 'includes/content.node_form');
  module_load_include('inc', 'content', 'includes/content.admin');
  // Get all of our form options such as day, time, room.
  
  // Define the field names we'd like to render.
  $render = array('field_session_day', 'field_session_time', 'field_session_room');
  $types = _camp_schedule_node_types();
  foreach ($types as $type) {
    $content_types[$type] = content_types($type);
  }

  drupal_add_js("Drupal.behaviors.camp_schedule = function() {
    $('tr', '#camp-schedule-manage-form').hover(function() {
      $(this).find('td').css('background-color', '#FFFCCC');
    }, function() {
      $(this).find('td').css('background-color', 'transparent');      
    });
    $('label', '#camp-schedule-manage-form').hide();
  }", 'inline');

  // Find all of our session nodes.
  $form['#tree'] = TRUE;
  $query = db_query("SELECT nid FROM {node} WHERE type IN (". db_placeholders($types, 'varchar') .") AND status = 1", $types);
  while ($row = db_fetch_object($query)) {
    $node = node_load($row->nid);
    
    $form['#node'] = $node;
    $form['nodes'][$node->nid]['title'] = array('#value' => l($node->title, 'node/'. $node->nid));

    foreach ($content_types[$node->type]['fields'] as $field_name => $field) {
      if (in_array($field_name, $render)) {
        $form['#field_info'][$field['field_name']] = $field;
        $form['nodes'][$node->nid] += content_field_form($form, $form_state, $field);
      }
    }
  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Save',
  );
  return $form;
}

/**
 * Form theme function.
 */
function theme_camp_schedule_manage_form($form) {
  $header = array('Session Title', 'Day', 'Room', 'Time');
  if (is_array($form['nodes'])) {
    foreach ($form['nodes'] as $nid => $node) {
      if (is_numeric($nid)) {
        $rows[] = array(
          drupal_render($form['nodes'][$nid]['title']),
          drupal_render($form['nodes'][$nid]['field_session_day']),
          drupal_render($form['nodes'][$nid]['field_session_room']),
          drupal_render($form['nodes'][$nid]['field_session_time'])
        );
      }
    }
  }
  $output .= theme('table', $header, $rows);
  $output .= drupal_render($form);
  return $output;
}

/**
 * Validation callback.
 */
function camp_schedule_manage_form_validate($form, &$form_state) {
  foreach ($form_state['values']['nodes'] as $nid => $value) {
    if (is_numeric($nid)) {
      $room = $form_state['values']['nodes'][$nid]['field_session_room'][0]['value'];
      $day = $form_state['values']['nodes'][$nid]['field_session_day'][0]['value'];
      $time = $form_state['values']['nodes'][$nid]['field_session_time'][0]['value'];
      if ($room && $day && $time) {
        $values[$room][$day][$time] = TRUE;
        if ($values[$room][$day][$time]) {
          // form_set_error('nodes]['. $nid .'][field_session_time', 'Scheduling conflict');
        }        
      }
    }
  }
  // var_dump($values);
}

/**
 * Submit callback.
 */
function camp_schedule_manage_form_submit($form, &$form_state) {
  foreach ($form_state['values']['nodes'] as $nid => $value) {
    if (is_numeric($nid)) {
      $node = node_load($nid);
      $node->field_session_day[0]['value'] = $form_state['values']['nodes'][$nid]['field_session_day'][0]['value'];
      $node->field_session_time[0]['value'] = $form_state['values']['nodes'][$nid]['field_session_time'][0]['value'];
      $node->field_session_room[0]['value'] = $form_state['values']['nodes'][$nid]['field_session_room'][0]['value'];
      node_save($node);
    }
  }
}

/**
 * 
 */
function _camp_schedule_node_types() {
  return array_filter(variable_get('camp_schedule_node_types', array()));
}