<?php
// vim:fenc=utf-8:ft=php:ai:si:ts=2:sw=2:et:nu:fdm=indent:fdn=1:

/**
 * Menu callback:
 * Administer addressbooks.
 */
function smilodon_card_admin() {
  $page = array();
  $header = array(
    'name' => array('data' => t('Name'), 'type' => 'property', 'specifier' => 'name', 'sort' => 'asc'),
    'mach_name' => array('data' => t('Machine name'), 'type' => 'property', 'specifier' => 'mach_name', 'sort' => 'asc'),
    'entity' => array('data' => t('Entity'), 'type' => 'property', 'specifier' => 'entity'),
    'bundle' => array('data' => t('Bundle'), 'type' => 'property', 'specifier' => 'bundle'),
    'ops' => array('data' => t('Operations')),
  );

  $books = array();
  foreach (smilodon_card_addressbook_load_multiple(FALSE) as $book) {
    $ops = array(
      array('title' => t('Edit'), 'href' => 'admin/config/system/smilodon_card/'.$book->aid),
      array('title' => t('Mappings'), 'href' => 'admin/config/system/smilodon_card/' . $book->aid . '/mappings'),
      array('title' => t('Rebuild'), 'href' => 'admin/config/system/smilodon_card/' . $book->aid . '/rebuild'),
      array('title' => t('Delete'), 'href' => 'admin/config/system/smilodon_card/' . $book->aid . '/delete'),
    );

    $books[$book->aid] = array(
      'name' => $book->name,
      'mach_name' => $book->mach_name,
      'entity' => $book->entity,
      'bundle' => $book->bundle,
      'ops' => theme('links', array(
        'links' => $ops, 
        'attributes' => array('class' => array('links', 'inline')),
      )),
    );
  }

  $page['books'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $books,
    '#empty' => t('No Addressbooks available.'),
  );
  return $page;
}

/**
 * Helper function to check if an addressbook's mach_name already exists.
 */
function _smilodon_card_admin_mach_name_exists($mach_name) {
  return db_query('SELECT a.aid FROM {smilodon_card_addressbooks} a WHERE a.mach_name = :mach_name', array(':mach_name' => $mach_name))->rowCount();
}

/**
 * Menu callback:
 * Create / edit an addressbook.
 *
 * @param addressbook
 *  A fully loaded addressbook entity.
 */
function smilodon_card_admin_edit($form, &$form_state, $abook) {
  if (!isset($form_state['stage'])) $form_state['stage'] = 'basic_info';

  if (!isset($form['abook']))
    $form['abook'] = array(
      '#type' => 'value',
      '#value' => $abook,
    );

  switch ($form_state['stage']) {
  case 'basic_info':
    return smilodon_card_admin_edit_basic($form, $form_state, $abook);
    break;
  case 'mappings':
    return smilodon_card_admin_edit_mappings($form, $form_state, $abook);
    break;
  }


  return $form;
}

function smilodon_card_admin_edit_validate($form, &$form_state) {
  switch ($form_state['stage']) {
  case 'basic_info':
    return smilodon_card_admin_edit_basic_validate($form, $form_state);
    break;
  case 'mappings':
    return smilodon_card_admin_edit_mappings_validate($form, $form_state);
    break;
  }
}

function smilodon_card_admin_edit_submit($form, &$form_state) {
  switch ($form_state['stage']) {
  case 'basic_info':
    // Always jump to mappings when creating a new abook.
    if (!$form_state['values']['abook']->aid || $form_state['triggering_element']['#value'] === t('Edit mappings')) {
      $form_state['stage'] = 'mappings';
      $form_state['rebuild'] = TRUE;
    }
    smilodon_card_admin_edit_basic_submit($form, $form_state);
    break;

  case 'mappings':
    smilodon_card_admin_edit_mappings_submit($form, $form_state);
    break;
  }

  $form_state['redirect'] = 'admin/config/system/smilodon_card';
}

function smilodon_card_admin_edit_basic($form, &$form_state, $abook) {
  $options = array();
  foreach (entity_get_info() as $entity => $data) {
    if ($data['fieldable'] && !empty($data['bundles'])) {
      $options[$data['label']] = array();
      foreach ($data['bundles'] as $bundle => $bundle_data)
        $options[$data['label']][$entity . '|' . $bundle] = $bundle_data['label'];
    }
  }

  if (!isset($form['abook']))
    $form['abook'] = array(
      '#type' => 'value',
      '#value' => $abook,
    );

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#required' => TRUE,
    '#default_value' => $abook->name,
  );
  $form['mach_name'] = array(
    '#type' => 'machine_name',
    '#default_value' => $abook->mach_name,
    '#machine_name' => array(
      'exists' => '_smilodon_card_admin_mach_name_exists',
    ),
  );
  $form['component'] = array(
    '#type' => 'select',
    '#title' => t('Base entity'),
    '#description' => t('Select the entity and bundle on which this addressbook is based on.'),
    '#default_value' => $abook->entity . '|' . $abook->bundle,
    '#options' => $options,
    '#required' => TRUE,
    '#disabled' => $abook->aid ? TRUE : FALSE,
  );
  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => 'Description',
    '#maxlength' => 255,
    '#default_value' => $abook->description,
  );

  $form['actions'] = array('#type' => 'actions', '#weight' => 99);
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  if ($abook->aid)
    $form['actions']['mappings'] = array(
      '#type' => 'submit',
      '#value' => t('Edit mappings'),
    );
  $form['actions']['cancel'] = array(
    '#markup' => l(t('Cancel'), 'admin/config/system/smilodon_card'),
    '#weight' => 99,
  );

  return $form;
}

function smilodon_card_admin_edit_basic_validate($form, &$form_state) {
  // Stub.
}

function smilodon_card_admin_edit_basic_submit($form, &$form_state) {
    $abook = $form_state['values']['abook'];
    $abook->name = filter_xss($form_state['values']['name'], array());
    $abook->mach_name = filter_xss($form_state['values']['mach_name'], array());
    $abook->description = filter_xss($form_state['values']['description'], array());
    list($abook->entity, $abook->bundle) = explode('|', $form_state['values']['component']);

    smilodon_card_addressbook_save($abook);
    form_set_value($form['abook'], $abook, $form_state);
    drupal_set_message(t('Addressbook %abook saved.', array('%abook' => $abook->name)));
}

/**
 * Menu callback:
 * Administer field mappings for an addressbook.
 *
 * @param $abook
 *  The fully loaded addressbook entity.
 */
function smilodon_card_admin_edit_mappings($form, &$form_state, $abook) {
  drupal_set_title(t('Edit mappings for @abook', array('@abook' => $abook->name)));

  // In case we're in a multi-step environment, pull the modified addressbook.
  if (isset($form_state['values']['abook']))
    $abook = $form_state['values']['abook'];

  // Map the available fields.
  $fields_list = field_info_instances($abook->entity, $abook->bundle);
  $field_data = array();
  foreach ($fields_list as $field_name => $data) {
    $field = field_info_field_by_id($data['field_id']);
    $field_data[$field_name] = $data['label'] . ' (' . $field_name . ')';
  }

  $labels = array(
    'basic' => t('Basic Information'),
    'N' => t('Name'),
    'HOME' => t('Private'),
    'WORK' => t('Work'),
    'OTHER' => t('Other'),
    'TEL' => t('Phone details'),
    'EMAIL' => t('E-Mail details'),
    'ADR' => t('Address details'),
    'NOTES' => t('Notes'),
  );

  $form['#tree'] = TRUE;

  if (!isset($form['abook']))
    $form['abook'] = array(
      '#type' => 'value',
      '#value' => $abook,
    );

  foreach (_smilodon_card_vcard_fields() as $group => $sections) {
    $form[$group . '_title'] = array(
      '#type' => 'item',
      '#title' => $labels[$group],
    );
    $form[$group] = array(
      '#type' => 'vertical_tabs',
    );
    foreach ($sections as $key => $data) {
      // Some vCard are comprised of multiple elements, so we need to render a field per element.
      if (is_array($data)) {
        $form[$group][$key] = array(
          '#type' => 'fieldset',
          '#title' => $labels[$key],
        );
        foreach ($data as $field => $title)
          $form[$group][$key][$field] = _smilodon_card_admin_build_sub_element($abook, $group, $key, $field, $title, $field_data);
      }
      else {
        $form[$group][$key] = _smilodon_card_admin_build_element($abook, $group, $key, $data, $field_data);
      }
    }
  }

  // Assign default settings.
  // This is a bit messy but so far the only practicable way to do it I could think of...
  $sources = array('field');
  if (module_exists('token'))
    $sources[] = 'token';

  foreach ($abook->mappings as $group => $fields) {
    foreach($fields as $field => $data) {
      // Normal field:
      if (in_array(key($data), $sources)) {
        $form[$group][$field]['source']['#default_value'] = key($data);
        $form[$group][$field][key($data)]['#default_value'] = reset($data);
      }
      else {
        // Loop through the sub-elements of a field (e.g. N, ADR):
        foreach ($data as $sub_field => $data) {
          $form[$group][$field][$sub_field]['source']['#default_value'] = key($data);
          $form[$group][$field][$sub_field][key($data)]['#default_value'] = reset($data);
        }
      }
    }
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}

function smilodon_card_admin_edit_mappings_validate($form, &$form_state) {
}

function smilodon_card_admin_edit_mappings_submit($form, &$form_state) {
  $mapping = array();
  $field_list = array();  // To make life a little easier, we have a dedicated setting containing just the fields.

  $vcard_fields = _smilodon_card_vcard_fields();

  foreach(array_keys(_smilodon_card_vcard_fields()) as $group) {
    foreach ($form_state['values'][$group] as $field => $data) {
      if (isset($data['source']) && $data['source']) {
        $mapping[$group][$field][$data['source']] = check_plain($data[$data['source']]);
        if ($data['source'] == 'field')
          $field_list[] = check_plain($data[$data['source']]);
      } else if (is_array($data)) {
      foreach ($data as $sub_field => $data)
        if (isset($data['source']) && $data['source']) {
          $mapping[$group][$field][$sub_field][$data['source']] = check_plain($data[$data['source']]);
          if ($data['source'] == 'field')
            $field_list[] = check_plain($data[$data['source']]);
        }
      }
    }
  }
  $abook = $form_state['values']['abook'];
  $abook->mappings = $mapping;
  smilodon_card_addressbook_save($abook);
# TODO: figure out what we needed this for...
#  variable_set('smilodon_fields', array_unique($field_list));
  drupal_set_message(t('The configuration options have been saved.'));
}

/**
 * Menu callback: Display a delete-form.
 */
function smilodon_card_admin_delete($form, &$form_state, $abook) {
  $form['abook'] = array(
    '#type' => 'value',
    '#value' => $abook,
  );
  return confirm_form($form, 
    t('Delete addressbook %abook?', array('%abook' => $abook->name)),
    'admin/config/system/smilodon_card',
    NULL,
    t('Delete'));
}
function smilodon_card_admin_delete_submit($form, &$form_state) {
  $abook = $form_state['values']['abook'];
  smilodon_card_addressbook_delete($abook->aid);
  drupal_set_message(t('Addressbook %abook deleted.', array('%abook' => $abook->name)));
  drupal_goto('admin/config/system/smilodon_card');
}

function smilodon_card_admin_rebuild($form, &$form_state, $abook = NULL) {
  $dest = drupal_get_destination();
  $dest = reset($dest);
  if (!$abook) {
    $abooks = array('all' => t('Rebuild all'));
    foreach (smilodon_card_addressbook_load_multiple(FALSE) as $abook)
      $abooks[$abook->aid] = $abook->name . ' (' . $abook->mach_name . ')';

    $form['aid'] = array(
      '#type' => 'select',
      '#options' => $abooks,
      '#title' => t('Select addressbook to be rebuilt'),
      '#required' => TRUE,
      '#multiple' => TRUE,
    );
    $form['actions'] = array('#type' => 'actions', );
    $form['actions']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Rebuild'),
    );
    $form['actions']['cancel'] = array(
      '#markup' => l(t('Cancel'), $dest === current_path() ? 'admin/config/system/smilodon_card' : $dest),
      '#weight' => 99,
    );

    return $form;
  } else {
    $form['aid'] = array(
      '#type' => 'value',
      '#value' => $abook->aid,
    );
    return confirm_form(
      $form, 
      t('Are you sure you want to rebuild the addressbook %addressbook?', array('%addressbook' => $abook->name)),
      $dest === current_path() ? 'admin/config/system/smilodon_card' : $dest,
      NULL);
  }

}

function smilodon_card_admin_rebuild_submit($form, &$form_state) {
  debug(__function__);
  $dest = drupal_get_destination();
  $dest = reset($dest);
  $abooks = array();
  $batch_ops = array(
    'operations' => array(),
    'finished' => '_smilodon_card_rebuild_finished',
    'title' => t('Rebuilding addressbooks'),
  );

  debug($form_state['values']);
  
  if (!in_array('all', $form_state['values']['aid']))
    $abooks = smilodon_card_addressbook_load_multiple($form_state['values']['aid']);
   else
    $abooks = smilodon_card_addressbook_load_multiple(FALSE);

  foreach ($abooks as $abook) {
    foreach (entity_load($abook->entity) as $entity) {
      // Chances are, users will be among the abooks. Skip the anonymous user.
      if ($abook->entity == 'user' && $entity->uid == 0)
        continue;
      $batch_ops['operations'][] = array('_smilodon_card_rebuild_card', array($abook, $entity));
    }
  }

  $form_state['redirect'] = $dest === current_path() ? 'admin/config/system/smilodon_card' : $dest;
  batch_set($batch_ops);
}

// HELPER FUNCTIONS

/**
 * Build the form elements for the mapper.
 *
 * @param $abook
 *  The addressbook.
 *
 * @param $group
 *  The internal group (FAPI).
 *
 * @param $mach_name
 *  The field name (FAPI).
 *
 * @param $title
 *  The field title.
 *
 * @param options
 *  If this parameter is passed, the function will return a select field with the given options.
 *
 */
function _smilodon_card_admin_build_element(&$abook, $group, $mach_name, $title, $options = array()) {
  $sources = array(0 => t('None'), 'field' => t('Field'));
  if (module_exists('token'))
    $sources['token'] = t('Token');

  $section = array();
  
  $section = array(
    '#type' => 'fieldset',
    '#title' => $title,
    '#description' => t('Select the source for vCard field.'),
  );
  $section['source'] = array(
    '#type' => 'radios',
    '#options' => $sources,
    '#default_value' => '0',
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );
  $section['field'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#states' => array(
      'visible' => array(
        ':input[name="' . $group . '[' . $mach_name . '][source]"]' => array('value' => 'field'),
      ),
    ),
  );

  if (isset($sources['token'])) {
    $section['token'] = array(
      '#type' => $mach_name == 'NOTES' ? 'textarea': 'textfield',
      '#description' => theme('token_tree', array('token_types' => array($abook->entity), 'global_types' => FALSE, 'dialog' => TRUE)),
      '#states' => array(
        'visible' => array(
          ':input[name="' . $group . '[' . $mach_name . '][source]"]' => array('value' => 'token'),
        ),
      ),
    );
  }
  $section['separator'] = array(
    '#type' => 'textfield',
    '#size' => 10,
    '#maxlength' => 10,
    '#defaul_value' => '',
    '#states' => array(
      'invisible' => array(
        ':input[name="' . $group . '[' . $mach_name. '][source]"]' => array('value' => 0),
      ),
    ),
  );

  return $section;
}

function _smilodon_card_admin_build_sub_element(&$abook, $group, $field, $element, $title, $options = array()) {
  $sources = array(0 => t('None'), 'field' => t('Field'));
  if (module_exists('token'))
    $sources['token'] = t('Token');

  $section = array();
  $section['title'] = array(
    '#type' => 'item',
    '#title' => $title,
  );
  $section['source'] = array(
    '#type' => 'radios',
    '#options' => $sources,
    '#default_value' => '0',
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );
  $section['field'] = array(
    '#type' => 'select',
    '#title' => t('Field'),
    '#title_display' => 'invisible',
    '#options' => $options,
    '#states' => array(
      'visible' => array(
        ':input[name="' . $group . '[' . $field . '][' . $element . '][source]"]' => array('value' => 'field'),
      ),
    ),
  );

  if (isset($sources['token'])) {
    $section['token'] = array(
      '#type' => $element == 'NOTES' ? 'textarea': 'textfield',
      '#title' => t('Token'),
      '#title_display' => 'invisible',
      '#description' => theme('token_tree', array('token_types' => array($abook->entity), 'global_types' => FALSE, 'dialog' => TRUE)),
      '#states' => array(
        'visible' => array(
          ':input[name="' . $group . '[' . $field . '][' . $element . '][source]"]' => array('value' => 'token'),
        ),
      ),
    );
  }

  $section['spacer'] = array(
    '#type' => 'item',
    '#markup' => '&nbsp;',
      '#states' => array(
        'visible' => array(
          ':input[name="' . $group . '[' . $field . '][' . $element . '][source]"]' => array('value' => 0),
        ),
      ),
  );

  return $section;
}
