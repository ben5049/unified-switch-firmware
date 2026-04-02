#define CHECK(func)                                    \
    do {                                               \
        status = (func);                               \
        sja1105_check_status_msg(&hsw0, status, true); \
    } while (0)


/* Private function prototypes */
// static void sja1105_check_status_msg(sja1105_handle_t *dev, sja1105_status_t to_check, bool recurse);

/* Attemt to handle errors resulting from SJA1105 user function calls
 * NOTE: When the system error handler is called, it is assumed that if it returns (as opposed to restarting the chip) then the error has been fixed.
 */
// static void sja1105_check_status_msg(sja1105_handle_t *dev, sja1105_status_t to_check, bool recurse) {
//
//     /* Return immediately if everything is fine */
//     if (to_check == SJA1105_OK) return;
//
//     sja1105_status_t sja_status       = SJA1105_OK;
//     bool             error_solved = false;
//
//     /* to_check is an error, increment the counter and check what to do */
//     sja1105_error_counter++;
//     switch (to_check) {
//
//         /* TODO: Log an error, but continue */
//         case SJA1105_ALREADY_CONFIGURED_ERROR:
//             error_solved = true;
//             break;
//
//         /* Parameter errors cannot be corrected on the fly, only at compile time. */
//         case SJA1105_PARAMETER_ERROR:
//             break;
//
//         /* If there is a CRC error then rollback to the default config */
//         case SJA1105_CRC_ERROR:
//             sja1105_static_conf      = swv4_sja1105_static_config_default;
//             sja1105_static_conf_size = SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE;
//             sja_status                   = SJA1105_ReInit(dev, sja1105_static_conf, sja1105_static_conf_size);
//             error_solved             = true;
//             break;
//
//         /* If there is an error with the static configuration load the default config */
//         case SJA1105_STATIC_CONF_ERROR:
//             sja1105_static_conf      = swv4_sja1105_static_config_default;
//             sja1105_static_conf_size = SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE;
//             sja_status                   = SJA1105_ReInit(dev, sja1105_static_conf, sja1105_static_conf_size);
//             error_solved             = dev->initialised;
//             break;
//
//         /* If there is a RAM parity error the switch must be immediately reset */
//         case SJA1105_RAM_PARITY_ERROR:
//             sja_status       = SJA1105_ReInit(dev, sja1105_static_conf, sja1105_static_conf_size);
//             error_solved = dev->initialised;
//             break;
//
//         /* Error has not been corrected */
//         default:
//             break;
//     }
//
//     /* A NEW ERROR has occured during the handling of the previous error... */
//     if (sja_status != SJA1105_OK) {
//         sja1105_error_counter++;
//
//         /* ...and the new error is the SAME as the previous error... */
//         if (sja_status == to_check) {
//
//             /* ...but the previous error was SOLVED: the new error is also solved */
//             if (error_solved)
//                 ;
//
//             /* ...and the previous error was NOT SOLVED: the problem is deeper, call the system error handler */
//             else
//                 error_handler();
//         }
//
//         /* ...and the new error is DIFFERENT from the previous error... */
//         else {
//
//             /* ...but the previous error was SOLVED: check the new error (recursively) */
//             if (error_solved) {
//                 if (recurse) {
//                     sja1105_error_counter--; /* Don't double count the new error */
//                     sja1105_check_status_msg(dev, sja_status, false);
//                 } else
//                     error_handler(); /* An error occurred while checking an error that occurred while checking an error. Yikes */
//             }
//
//             /* ...and the previous error was NOT SOLVED: the problem is deeper, call the system error handler */
//             else
//                 error_handler();
//         }
//         error_solved = true;
//     }
//
//     /* Unsolved error */
//     if (!error_solved) error_handler();
//
//     /* All errors have now been handled, check the sja_status registers just to be safe */
//     sja_status = SJA1105_CheckStatusRegisters(dev);
//     if (recurse) sja1105_check_status_msg(dev, sja_status, false);
//
//     /* An error occurring means the mutex could have been taken but not released. Release it now */
//     while (dev->callbacks->callback_give_mutex(dev) == SJA1105_OK);
// }
