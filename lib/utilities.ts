import { CppCas } from './binding'
import { DurabilityLevel } from './generaltypes'
import parseDuration from 'parse-duration'
import * as qs from 'querystring'

/**
 * CAS represents an opaque value which can be used to compare documents to
 * determine if a change has occurred.
 *
 * @category Key-Value
 */
export type Cas = CppCas

/**
 * Reprents a node-style callback which receives an optional error or result.
 *
 * @category Utilities
 */
export interface NodeCallback<T> {
  (err: Error | null, result: T | null): void
}

/**
 * @internal
 */
export class PromiseHelper {
  /**
   * @internal
   */
  static wrapAsync<T, U extends Promise<T>>(
    fn: () => U,
    callback?: (err: Error | null, result: T | null) => void
  ): U {
    // If a callback in in use, we wrap the promise with a handler which
    // forwards to the callback and return undefined.  If there is no
    // callback specified.  We directly return the promise.
    if (callback) {
      const prom = fn()
      prom
        .then((res) => callback(null, res))
        .catch((err) => callback(err, null))
      return prom
    }

    return fn()
  }

  /**
   * @internal
   */
  static wrap<T>(
    fn: (callback: NodeCallback<T>) => void,
    callback?: NodeCallback<T> | null
  ): Promise<T> {
    return new Promise((resolve, reject) => {
      fn((err, res) => {
        if (err) {
          if (callback) {
            callback(err, null)
          }

          reject(err as Error)
        } else {
          if (callback) {
            callback(null, res)
          }

          resolve(res as T)
        }
      })
    })
  }
}

/**
 * @internal
 */
export class CompoundTimeout {
  private _start: [number, number]
  private _timeout: number | undefined

  /**
   * @internal
   */
  constructor(timeout: number | undefined) {
    this._start = process.hrtime()
    this._timeout = timeout
  }

  /**
   * @internal
   */
  left(): number | undefined {
    if (this._timeout === undefined) {
      return undefined
    }

    const period = process.hrtime(this._start)

    const periodMs = period[0] * 1e3 + period[1] / 1e6
    if (periodMs > this._timeout) {
      return 0
    }

    return this._timeout - periodMs
  }

  /**
   * @internal
   */
  expired(): boolean {
    const timeLeft = this.left()
    if (timeLeft === undefined) {
      return false
    }

    return timeLeft <= 0
  }
}

/**
 * @internal
 */
export function msToGoDurationStr(ms?: number): string | undefined {
  if (ms === undefined) {
    return undefined
  }

  return `${ms}ms`
}

/**
 * @internal
 */
export function goDurationStrToMs(str?: string): number | undefined {
  if (str === undefined) {
    return undefined
  }

  const duration = parseDuration(str)
  if (duration === null) {
    return undefined
  }

  return duration
}

/**
 * @internal
 */
export function duraLevelToNsServerStr(
  level: DurabilityLevel | string | undefined
): string | undefined {
  if (level === undefined) {
    return undefined
  }

  if (typeof level === 'string') {
    return level as string
  }

  if (level === DurabilityLevel.None) {
    return 'none'
  } else if (level === DurabilityLevel.Majority) {
    return 'majority'
  } else if (level === DurabilityLevel.MajorityAndPersistOnMaster) {
    return 'majorityAndPersistActive'
  } else if (level === DurabilityLevel.PersistToMajority) {
    return 'persistToMajority'
  } else {
    throw new Error('invalid durability level specified')
  }
}

/**
 * @internal
 */
export function nsServerStrToDuraLevel(level: string): DurabilityLevel {
  if (level === 'none') {
    return DurabilityLevel.None
  } else if (level === 'majority') {
    return DurabilityLevel.Majority
  } else if (level === 'majorityAndPersistActive') {
    return DurabilityLevel.MajorityAndPersistOnMaster
  } else if (level === 'persistToMajority') {
    return DurabilityLevel.PersistToMajority
  } else {
    throw new Error('invalid durability level string')
  }
}

/**
 * @internal
 */
export function cbQsStringify(values: { [key: string]: any }): string {
  const cbValues: { [key: string]: any } = {}
  for (const i in values) {
    if (values[i] === undefined) {
      // skipped
    } else if (typeof values[i] === 'boolean') {
      cbValues[i] = values[i] ? 1 : 0
    } else {
      cbValues[i] = values[i]
    }
  }
  return qs.stringify(cbValues)
}
