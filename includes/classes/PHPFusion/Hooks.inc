<?php
/*-------------------------------------------------------+
| PHP-Fusion Content Management System
| Copyright (C) 2002 - 2013 Nick Jones
| http://www.phpfusion.com/
+--------------------------------------------------------+
| Filename: Hooks.inc
| Author: Frederick Chan (Deviance)
+--------------------------------------------------------+
| This program is released as free software under the
| Affero GPL license. You can redistribute it and/or
| modify it under the terms of this license which you
| can read by viewing the included agpl.txt or online
| at www.gnu.org/licenses/agpl.html. Removal of this
| copyright header is strictly prohibited without
| written permission from the original author(s).
+--------------------------------------------------------*/
namespace PHPFusion;

/**
 * Class Hooks
 * Core class for storing, removing and run hook functions.
 *
 * @package PHPFusion
 */
final class Hooks {

    private $hooks = [];

    private static $instances = NULL;

    private $output = array();

    /**
     * Get an instance by key
     *
     * @param string $key
     *
     * @return static
     */
    public static function get_instances($key = 'default') {
        if (!isset(self::$instances[$key])) {
            self::$instances[$key] = new static();
        }

        return self::$instances[$key];
    }

    /**
     * Register a hook into the $instance
     *
     * @param $filter_name
     * @param $function
     * @param $que
     * @param $default_args
     * @param $accepted_args
     *
     * @return bool
     */
    public function add_hook($filter_name, $function, $que, $default_args, $accepted_args) {
        $hooks = array(
            'function'      => $function,
            'default_args'  => $default_args,
            'accepted_args' => $accepted_args,
            'que'           => $que,
        );
        $this->hooks[$que][$filter_name][] = $hooks;
        if (count($this->hooks) > 1) {
            ksort($this->hooks, SORT_NUMERIC);
        }

        return TRUE;
    }

    /**
     * Returns the hook by $filter_name and $function
     *
     * @param        $filter_name
     * @param string $function
     *
     * @return array
     */
    public function get_hook($filter_name, $function = '') {
        if (!empty($this->hooks)) {
            array_filter($this->hooks);
            foreach ($this->hooks as $hooks) {
                if (!empty($hooks)) {
                    if (isset($hooks[$filter_name])) {
                        if ($function == 'invalid_notices') {
                            return array();
                        }
                        if (!empty($function)) {
                            return (array)($hooks[$filter_name]['function'] == $function ? $hooks[$filter_name]['function'] : array());
                        } else {
                            return (array)$hooks[$filter_name];
                        }
                    }
                }
            }
        }

        return array();
    }

    /**
     * Remove a specified hook from the $instance
     *
     * @param $filter_name
     * @param $function
     * @param $que
     *
     * @return bool
     */
    public function remove_hook( $filter_name, $function = '', $que ) {

        if ( $function ) {
            if ( isset( $this->hooks[ $que ][ $filter_name ] ) ) {
                foreach ( $this->hooks[ $que ][ $filter_name ] as $key => $hooks ) {
                    if ( $hooks['function'] == $function ) {
                        unset( $this->hooks[ $que ][ $filter_name ][ $key ] );
                        if ( empty( $this->hooks[ $que ][ $filter_name ] ) )
                            unset( $this->hooks[ $que ][ $filter_name ] );

                        return TRUE;
                    }
                }
            }
        }

        unset( $this->hooks[ $que ][ $filter_name ] );

        return TRUE;
    }

    /**
     * Remove all hooks from the $instance
     *
     * @param bool $que
     */
    public function remove_all_hook($que = FALSE) {

        if ($que === false) {
            $this->hooks = [];

        } elseif (isset($this->hooks[$que])) {
            unset($this->hooks[$que]);
        }

    }


    /**
     * Run the hooks by $filter_name and $args parameters
     * There will be no output. If you need an output, use filter hook.
     * @param $filter_name
     *
     * @throws \Exception
     */
    public function apply_hook($filter_name, $remove_after_access = TRUE) {

        $function_args = func_get_args();
        $current_hook = $this->get_hook($filter_name);

        if (!empty($current_hook)) {

            foreach ($current_hook as $hook) {

                // prevent the current hook from being called twice, executed or not, else crash
                if ($remove_after_access) {
                    $this->remove_hook($filter_name, $hook['function'], $hook['que']);
                }

                if (function_exists($hook['function'])) {

                    $args = (!empty($hook['default_args']) ? $hook['default_args'] : array());

                    $_callback_args = FALSE;

                    if (count($function_args) > 1) {
                        unset($function_args[0]);
                        $args = $function_args;
                        $_callback_args = TRUE;
                    }

                    if ($hook['accepted_args']) {
                        if ($hook['accepted_args'] < (count($function_args) - 1 ) ) {
                            throw new \Exception("Too many arguments during executing the $filter_name hook");
                        }
                    }

                    $output = $_callback_args === FALSE ? $hook['function']($args) : call_user_func_array($hook['function'], $args);

                    if (!empty($output)) {

                        $this->output[$filter_name][] = $output;

                    }
                }
            }

            if (!empty($this->get_hook($filter_name))) $this->apply_hook($filter_name, $function_args);
        }
    }

    /**
     * Run the hook filter, can be used multiple times within a loop to get the parse.
     * @param $filter_name
     *
     * @return mixed|string
     * @throws \Exception
     */
    public function apply_hook_once($filter_name) {
        $function_args = func_get_args();
        $current_hook = $this->get_hook($filter_name);
        if (!empty($current_hook)) {
            foreach ($current_hook as $hook) {
                if (function_exists($hook['function'])) {
                    $args = (!empty($hook['default_args']) ? $hook['default_args'] : []);
                    if (count($function_args) > 1) {
                        unset($function_args[0]);
                        $args = $function_args;
                    }
                    if ($hook['accepted_args']) {
                        if ($hook['accepted_args'] < (count($function_args) - 1 ) ) {
                            throw new \Exception("Too many arguments during executing the $filter_name hook");
                        }
                    }
                    $output = call_user_func_array($hook['function'], $args);

                    // remove the hook
                    $this->remove_hook( $filter_name, $hook['function'], $hook['que'] );

                    if (!empty($output)) {
                        return $output;
                    }
                }
            }
        }
        return '';
    }

    /**
     * Run the hook filter, can be used multiple times within a loop to get the parse.
     *
     * @param $filter_name
     *
     * @return mixed|string
     * @throws \Exception
     */
    public function repeat_current_hook( $filter_name ) {
        $function_args = func_get_args();
        $current_hook = $this->get_hook( $filter_name );
        if ( !empty( $current_hook ) ) {
            foreach ( $current_hook as $hook ) {
                if ( function_exists( $hook['function'] ) ) {
                    $args = ( !empty( $hook['default_args'] ) ? $hook['default_args'] : [] );
                    if ( count( $function_args ) > 1 ) {
                        unset( $function_args[0] );
                        $args = $function_args;
                    }
                    if ( $hook['accepted_args'] ) {
                        if ( $hook['accepted_args'] < ( count( $function_args ) - 1 ) ) {
                            throw new \Exception( "Too many arguments during executing the $filter_name hook" );
                        }
                    }
                    $output = call_user_func_array( $hook['function'], $args );
                    // remove the hook
                    //$this->remove_hook( $filter_name, $hook['function'], $hook['que'] );
                    if ( !empty( $output ) ) {
                        return $output;
                    }
                }
            }
        }
        return '';
    }

    /**
     * Run the hooks by $filter_name and $args parameters
     * This filter must only run once in application.
     *
     * @param $filter_name
     *
     * @return array|mixed
     */
    public function filter_hook($filter_name) {

        $function_args = func_get_args();
        call_user_func_array([$this, 'apply_hook'], $function_args);

        if (!empty($this->output[$filter_name]) && is_array($this->output[$filter_name])) {
            return $this->output[$filter_name];
        }

        return array();
    }

    /**
     * @param $filter_name
     *
     * @return string
     */
    public function filter_hook_once($filter_name) {

        $output = call_user_func_array([$this, 'apply_hook_once'], func_get_args());

        return (string)$output;
    }

    /**
     * @param $filter_name
     *
     * @return string
     */
    public function filter_hook_repeat($filter_name) {
        $output = call_user_func_array( [ $this, 'repeat_current_hook' ], func_get_args() );

        return (string)$output;
    }


    public function apply_all_hook() {

        if (!empty($this->hooks)) {

            foreach ($this->hooks as $que => $funcs_) {

                if (!empty($funcs_['function']) && function_exists($funcs_['function'])) {

                    call_user_func_array($funcs_['function'], $funcs_['accepted_args']);

                    array_shift($this->hooks[$que]);

                }
            }
        }
    }

}
