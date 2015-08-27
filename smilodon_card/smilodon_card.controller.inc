<?php
// vim:fenc=utf-8:ft=php:ai:si:ts=2:sw=2:et:nu:fdm=indent:fdn=1:

/**
 * @file
 * Smilodon CardDAV Field Storage backend.
 * This file contains the entity controllers for vCards and Addressbooks.
 *
 * @author Daniel Zauner <delta@fratler.de>
 * @copyright (c) Daniel Zauner <delta@fratler.de>
 */

/**
 * vCard Entity Controller.
 *
 * This is pretty straight-forward.
 */
class SmilodonCardController extends DrupalDefaultEntityController {
  function __construct($entityType) {
    parent::__construct($entityType);

    // Initialize SabreDAV.
    try {
      $lib = libraries_load('sabredav');

      if ($lib && !empty($lib['loaded'])) 
        return;

      if ($lib)
        throw new Exception($lib['error message']);
      else
        throw new Exception(t('Could not find SabreDAV.'));
    } catch (Exception $e) {
      watchdog_exception('smilodon_card', $e);
      throw $e;
    }
  }

  /**
   * Create and return a blank entity.
   */
  public function create() {

    $vcard = new stdClass();
    $vcard->cid = 0;
    $vcard->aid = 0;
    $vcard->ent_id = 0;
    $vcard->ent_vid = 0;
    $vcard->data = new Sabre\VObject\Component\VCard(array('PRODID' => '-//Smilodon Card v0.1//Sabre VObject ' . Sabre\VObject\Version::VERSION  . '//EN'));
    $vcard->etag = NULL;
    $vcard->uuid = NULL;
    $vcard->lastmod = 0;
    $vcard->size = 0;

    return $vcard;
  }

  /**
   * Load the vCards.
   * We can rely on the default loader, all we need to do is rendering the data field.
   */
  public function load($ids = array(), $conditions = array()) {
    $cards = parent::load($ids, $conditions);

    foreach ($cards as &$card) 
      $card->data = Sabre\VObject\Reader::read($card->data);

    # TODO: In case we should ever be crazy enough to offer write-support for clients, 
    #       consider using Sabre\VObject\Reader::OPTION_FORGIVING as 2nd para.
    #       See: http://sabre.io/vobject/usage/ (Outlook)

    return $cards;
  }

  public function save($vcard) {
    // Make sure the UID is in there, otherwise iOS/OSX will freak out.
    $vcard->data->UID = $vcard->uuid;
    module_invoke_all('entity_presave', $vcard, 'smilodon_card_vcard');

    // In case we're updating: See if anything has changed and set the timestamp accordingly.
    if ($vcard->cid) {
      $vcard_old = explode("\r\n", $vcard->original->data->serialize());
      $vcard_new = explode("\r\n", $vcard->data->serialize());
      $data_added = array_diff($vcard_new, $vcard_old);
      $data_removed = array_diff($vcard_old, $vcard_new);
      if (!empty($data_added)) {
        $vcard->lastmod = REQUEST_TIME;
#        debug($data_added, 'TODO: Added data');
      } 
      if (!empty($data_removed)) {
        $vcard->lastmod = REQUEST_TIME;
#        debug($data_removed, 'TODO: Removed data');
      }
    } else {
#      debug('Creating new card...');
      $vcard->lastmod = REQUEST_TIME;
    }

#    $data = explode("\r\n", $vcard->data->serialize());
#    dpm($data, 'saving this vCard');

    // This is basically a courtesy function; in case the write-operation fails, we always have the carddata field.
    $vcard->size = smilodon_card_write($vcard);

    // Calculate eTag.
    $vcard->etag = implode('-', array($vcard->cid, $vcard->aid, $vcard->lastmod));

    $state = db_merge('smilodon_card_vcards')
      ->key(array('cid' => $vcard->cid))
      ->fields(array(
        'aid' => $vcard->aid,
        'ent_id' => $vcard->ent_id,
        'ent_vid' => $vcard->ent_vid,
        'data' => (string)$vcard->data->serialize(),
        'etag' => $vcard->etag,
        'uuid' => $vcard->uuid,
        'lastmod' => $vcard->lastmod,
        'size' => $vcard->size,
      ))->execute();

    if ($state == MergeQuery::STATUS_INSERT)
      $vcard->cid = Database::getConnection()->lastInsertId();

    // @todo: we be needing a module_invoke_all()?!
    
    return $state;
  }

  public function delete($cid) {
    $this->deleteMultiple(array($cid));
  }

  public function deleteMultiple($cids) {
    $transaction = db_transaction();

    try {
      $vcards = $this->load($cids);
      foreach ($vcards as $vcard)
        smilodon_card_delete($vcard);

      db_delete('smilodon_card_vcards')
        ->condition('cid', $cids, 'IN')
        ->execute();

    } catch (Exception $e) {
      $transaction->rollback();
      watchdog('smilodon_card', 'Error while trying to delete a vCard: ' . $e);
    }
  }
}

/**
 * Addressbook Entity Controller
 */
class SmilodonCardAddressbookController extends DrupalDefaultEntityController {
  /**
   * Create and return a blank addressbook entity.
   */
  public function create() {
    $abook = new stdClass();
    $abook->aid = 0;
    $abook->name = NULL;
    $abook->mach_name = NULL;
    $abook->description = NULL;
    $abook->synctoken = 0;
    $abook->entity = NULL;
    $abook->bundle = NULL;
    $abook->mappings = array(); 

    return $abook;
  }

  /**
   * Load the addressbook.
   * We can rely on the default loader, all we need to do is deserialize the mappings.
   */
  public function load($ids = array(), $conditions = array()) {
    $abooks = parent::load($ids, $conditions);

    foreach ($abooks as &$abook) 
      if (!is_array($abook->mappings))
        $abook->mappings = unserialize($abook->mappings);

    return $abooks;
  }

  /**
   * Dump all the stuff in the right corners.
   *
   * @param $abook
   *  A fully-populated addressbook entity.
   *
   * @return
   *  The saved entity, including timestamp and ID.
   */
  public function save(&$abook) {
      $state = db_merge('smilodon_card_addressbooks')
        ->key(array('aid' => $abook->aid))
        ->fields(array(
          'name' => $abook->name,
          'mach_name' => $abook->mach_name,
          'description' => $abook->description,
          'synctoken' => $abook->synctoken,
          'entity' => $abook->entity,
          'bundle' => $abook->bundle,
          'mappings' => serialize($abook->mappings),
        ))->execute();

      if ($state == MergeQuery::STATUS_INSERT)
        $abook->aid = Database::getConnection()->lastInsertId();

      return $abook;
  }

  /**
   * Delete an addressbook.
   * This just wraps the ID in an array and passes it on.
   *
   * @param $aid
   *  An addressbook ID.
   *
   * @see SmilodonCardAddressbookController::deleteMultiple()
   */
  public function delete($aid) {
    $this->deleteMultiple(array($aid));
  }

  public function deleteMultiple($aids) {
    $transaction = db_transaction();

    try {
      // Remove the addressbook from the table.
      $query = db_delete('smilodon_card_addressbooks')
        ->condition('aid', $aids, 'IN')
        ->execute();

      // Remove all vCards associated with this addressbook.
      $efq = new EntityFieldQuery();
      $efq->entityCondition('entity_type', 'smilodon_card_vcard')
        ->propertyCondition('aid', $aids, 'IN');
      $efq = $efq->execute();

      if (isset($efq['smilodon_card_vcard'])) {
        $cids = array_keys($efq['smilodon_card_vcard']);
        entity_get_controller('smilodon_card_vcard')->deleteMultiple($cids);
      }

      // @todo If we're using the smilodon_card_addressbookchanges table, drop all changes from it as well.

    } catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('smilodon_card', $e);
      throw $e;
    }
  }
}
