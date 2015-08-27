<?php
/* vim: set filetype=php expandtab tabstop=2 shiftwidth=2 autoindent smartindent: */

/**
 * @file
 * Smilodon <-> SabreDAV interfaces and helper-functions.
 */

function _smilodon_card_filename(&$vcard) {
  return variable_get('smilodon_card_folder') . '/' . $vcard->uuid . '.vcf';
}

/**
 * Writes a vcf file to the designated folder.
 *
 * @param $vcard
 *  A fully-populated vCard object.
 */
function smilodon_card_write(&$vcard) {
  $state = file_unmanaged_save_data($vcard->data->serialize(), _smilodon_card_filename($vcard), FILE_EXISTS_REPLACE);
  if ($state)
    return filesize($state);
  else
    return $state;
}

/**
 * Remove a vcf file.
 *
 * @param $vcard
 *  A fully-populated vCard object.
 */
function smilodon_card_delete($vcard) {
  return file_unmanaged_delete(_smilodon_card_filename($vcard));
}
