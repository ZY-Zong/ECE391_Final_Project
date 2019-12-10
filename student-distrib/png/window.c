//
// Created by Zhenyu Zong on 2019/12/9.
//

/*
 * Assume the terminal is printed at (x, y). Window status bar should start at (x-6, y-21).
 * Window block consists of:
 *   |  name       |   size   |    position    |
 *      above        652 * 21    (x-6, y-21 )
 *      down         652 *  4    (x-6, y+480)
 *      left         6  * 480    (x-6, y    )
 *      right        6  * 480    (x+640, y  )
 *      red_b
 *      yellow_b
 *      green_b
 */

