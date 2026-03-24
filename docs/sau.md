# SAU Config

The Secure Attribution Unit (SAU) config is in `Core/Inc/partition_stm32h573xx.h`. There is a bug in the memory management tool (MMT) that can cause it to generate invalid default configurations. Change it to be like the following:

```C

/*
//   <e>Initialize SAU Region 5
//   <i> Setup SAU Region 5 memory attributes
*/
#define SAU_INIT_REGION5 1

/*
//     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START5 0x40000000 /* start address of SAU region 5 */

/*
//     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END5 0x400363FF /* end address of SAU region 5 */
/*
//     <o>Region is
//         <0=>Non-Secure
//         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC5 0
/*
//   </e>
*/

/*
//   <e>Initialize SAU Region 6
//   <i> Setup SAU Region 6 memory attributes
*/
#define SAU_INIT_REGION6 1

/*
//     <o>Start Address <0-0xFFFFFFE0>
*/
#define SAU_INIT_START6 0x40037400 /* start address of SAU region 6 */

/*
//     <o>End Address <0x1F-0xFFFFFFFF>
*/
#define SAU_INIT_END6 0x4FFFFFFF /* end address of SAU region 6 */
/*
//     <o>Region is
//         <0=>Non-Secure
//         <1=>Secure, Non-Secure Callable
*/
#define SAU_INIT_NSC6 0
/*
//   </e>
*/

```