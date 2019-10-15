#!/usr/bin/env python

import sys, os, getopt, sniper_lib


def generate_simout(jobid = None, resultsdir = None, partial = None, output = sys.stdout, silent = False):

  try:
    res = sniper_lib.get_results(jobid = jobid, resultsdir = resultsdir, partial = partial)
  except (KeyError, ValueError), e:
    if not silent:
      print 'Failed to generated sim.out:', e
    return

  results = res['results']
  config = res['config']
  ncores = int(config['general/total_cores'])

  format_int = lambda v: str(long(v))
  format_pct = lambda v: '%.1f%%' % (100. * v)
  def format_float(digits):
    return lambda v: ('%%.%uf' % digits) % v
  def format_ns(digits):
    return lambda v: ('%%.%uf' % digits) % (v/1e6)
  def format_ms(digits):
    return lambda v: ('%%.%uf' % digits) % (v/1e12)

  if 'barrier.global_time_begin' in results:
    time0_begin = results['barrier.global_time_begin']
    time0_end = results['barrier.global_time_end']

  if 'barrier.global_time' in results:
    time0 = results['barrier.global_time'][0]
  else:
    time0 = time0_begin - time0_end

  if sum(results['performance_model.instruction_count']) == 0:
    # core.instructions is less exact, but in cache-only mode it's all there is
    results['performance_model.instruction_count'] = results['core.instructions']

  results['performance_model.elapsed_time_fixed'] = [
    time0
    for c in range(ncores)
  ]
  results['performance_model.cycle_count_fixed'] = [
    results['performance_model.elapsed_time_fixed'][c] * results['fs_to_cycles_cores'][c]
    for c in range(ncores)
  ]
  results['performance_model.ipc'] = [
    i / (c or 1)
    for i, c in zip(results['performance_model.instruction_count'], results['performance_model.cycle_count_fixed'])
  ]
  results['performance_model.nonidle_elapsed_time'] = [
    results['performance_model.elapsed_time'][c] - results['performance_model.idle_elapsed_time'][c]
    for c in range(ncores)
  ]
  results['performance_model.idle_elapsed_time'] = [
    time0 - results['performance_model.nonidle_elapsed_time'][c]
    for c in range(ncores)
  ]
  results['performance_model.idle_elapsed_percent'] = [
    results['performance_model.idle_elapsed_time'][c] / float(time0)
    for c in range(ncores)
  ]

  template = [
    ('  Instructions', 'performance_model.instruction_count', str),
    ('  Cycles',       'performance_model.cycle_count_fixed', format_int),
    ('  IPC',          'performance_model.ipc', format_float(2)),
    ('  Time (ns)',    'performance_model.elapsed_time_fixed', format_ns(0)),
    ('  Idle time (ns)', 'performance_model.idle_elapsed_time', format_ns(0)),
    ('  Idle time (%)',  'performance_model.idle_elapsed_percent', format_pct),
  ]

  if 'branch_predictor.num-incorrect' in results:
    results['branch_predictor.missrate'] = [ 100 * float(results['branch_predictor.num-incorrect'][core])
      / ((results['branch_predictor.num-correct'][core] + results['branch_predictor.num-incorrect'][core]) or 1) for core in range(ncores) ]
    results['branch_predictor.mpki'] = [ 1000 * float(results['branch_predictor.num-incorrect'][core])
      / (results['performance_model.instruction_count'][core] or 1) for core in range(ncores) ]
    template += [
      ('Branch predictor stats', '', ''),
      ('  num correct',  'branch_predictor.num-correct', str),
      ('  num incorrect','branch_predictor.num-incorrect', str),
      ('  misprediction rate', 'branch_predictor.missrate', lambda v: '%.2f%%' % v),
      ('  mpki', 'branch_predictor.mpki', lambda v: '%.2f' % v),
    ]

  template += [
    ('TLB Summary', '', ''),
  ]

  for tlb in ('itlb', 'dtlb', 'stlb'):
    if '%s.access'%tlb in results:
      results['%s.missrate'%tlb] = map(lambda (a,b): 100*a/float(b or 1), zip(results['%s.miss'%tlb], results['%s.access'%tlb]))
      results['%s.mpki'%tlb] = map(lambda (a,b): 1000*a/float(b or 1), zip(results['%s.miss'%tlb], results['performance_model.instruction_count']))
      template.extend([
        ('  %s' % {'itlb': 'I-TLB', 'dtlb': 'D-TLB', 'stlb': 'L2 TLB'}[tlb], '', ''),
        ('    num accesses', '%s.access'%tlb, str),
        ('    num misses', '%s.miss'%tlb, str),
        ('    miss rate', '%s.missrate'%tlb, lambda v: '%.2f%%' % v),
        ('    mpki', '%s.mpki'%tlb, lambda v: '%.2f' % v),
      ])

  template += [
    ('Cache Summary', '', ''),
  ]
  allcaches = [ 'L1-I', 'L1-D' ] + [ 'L%u'%l for l in range(2, 5) ]
  existcaches = [ c for c in allcaches if '%s.loads'%c in results ]
  for c in existcaches:
    results['%s.accesses'%c] = map(sum, zip(results['%s.loads'%c], results['%s.stores'%c]))
    results['%s.misses'%c] = map(sum, zip(results['%s.load-misses'%c], results.get('%s.store-misses-I'%c, results['%s.store-misses'%c])))
    results['%s.missrate'%c] = map(lambda (a,b): 100*a/float(b) if b else float('inf'), zip(results['%s.misses'%c], results['%s.accesses'%c]))
    results['%s.mpki'%c] = map(lambda (a,b): 1000*a/float(b) if b else float('inf'), zip(results['%s.misses'%c], results['performance_model.instruction_count']))
    template.extend([
      ('  Cache %s'%c, '', ''),
      ('    num cache accesses', '%s.accesses'%c, str),
      ('    num cache misses', '%s.misses'%c, str),
      ('    miss rate', '%s.missrate'%c, lambda v: '%.2f%%' % v),
      ('    mpki', '%s.mpki'%c, lambda v: '%.2f' % v),
    ])

  allcaches = [ 'nuca-cache', 'dram-cache' ]
  existcaches = [ c for c in allcaches if '%s.reads'%c in results ]
  for c in existcaches:
    results['%s.accesses'%c] = map(sum, zip(results['%s.reads'%c], results['%s.writes'%c]))
    results['%s.misses'%c] = map(sum, zip(results['%s.read-misses'%c], results['%s.write-misses'%c]))
    results['%s.missrate'%c] = map(lambda (a,b): 100*a/float(b) if b else float('inf'), zip(results['%s.misses'%c], results['%s.accesses'%c]))
    icount = sum(results['performance_model.instruction_count'])
    icount /= len([ v for v in results['%s.accesses'%c] if v ]) # Assume instructions are evenly divided over all cache slices
    results['%s.mpki'%c] = map(lambda a: 1000*a/float(icount) if icount else float('inf'), results['%s.misses'%c])
    template.extend([
      ('  %s cache'% c.split('-')[0].upper(), '', ''),
      ('    num cache accesses', '%s.accesses'%c, str),
      ('    num cache misses', '%s.misses'%c, str),
      ('    miss rate', '%s.missrate'%c, lambda v: '%.2f%%' % v),
      ('    mpki', '%s.mpki'%c, lambda v: '%.2f' % v),
    ])

  # Uncore
  if 'L3.uncore-totaltime' in results:
    #results['dram-queues'] = map(sum(results['L3.uncore-time-dram-queue'])
    results['dram-queues'] = [
        sum(results['L3.uncore-time-dram-queue']) if c == 0 else float('inf')
        for c in range(ncores)
    ]
    results['dram-queues-avg'] = [
        sum(results['L3.uncore-time-dram-queue'])/len(results['L3.uncore-time-dram-queue']) if c == 0 else float('inf')
        for c in range(ncores)
    ]
    results['dram-queues-total-requests'] = [
        sum(results['dram-queue.num-requests']) if c == 0 else float('inf')
        for c in range(ncores)
    ]
    template += [
        ('L3 Uncore summary', '', ''),
        ('  num requests', 'L3.uncore-requests', str),
        ('  dram-bus time (ms)', 'L3.uncore-time-dram-bus', format_ms(2)),
        ('  dram-device time (ms)', 'L3.uncore-time-dram-device', format_ms(2)),
        ('  dram-queue uncore-time (ms)', 'L3.uncore-time-dram-queue', format_ms(2)),
        ('  uncore time (ms)', 'L3.uncore-totaltime', format_ms(2)),
        ('  dram-queue num requests', 'dram-queue.num-requests', str),
        ('  dram-queue total requests', 'dram-queues-total-requests', str),
        # dram-queue total delay is the same as in the DRAM summary section
        #('  dram-queue total delay (ms)', 'dram-queue.total-queue-delay', format_ms(2)),
        ('  dram-queue total-time-used (ms)', 'dram-queue.total-time-used', format_ms(2)),
        ('  total time-dram-queue (ms)','dram-queues', format_ms(2)),
        ('  avg time-dram-queue per cache (ms)','dram-queues-avg', format_ms(2)),
    ]
  #results['uncore.dram-queue'] = map(sum, zip(results['dram.reads'], results['dram.writes'])) 
  results['dram.accesses'] = map(sum, zip(results['dram.reads'], results['dram.writes']))
  results['dram.avglatency'] = map(lambda (a,b): a/b if b else float('inf'), zip(results['dram.total-access-latency'], results['dram.accesses']))
  template += [
    ('DRAM summary', '', ''),
    ('  num dram accesses', 'dram.accesses', str),
    ('  average dram access latency (ns)', 'dram.avglatency', format_ns(2)),
    ('  total dram access latency (ms)', 'dram.total-access-latency', format_ms(2)),
  ]
  if 'dram.total-read-queueing-delay' in results:
    #print("=== DRAM TOTAL_READ_Q_DELAY: %s" % results['dram.total-read-queueing-delay'])
    results['dram.avgqueueread'] = map(lambda (a,b): a/(b or 1), zip(results['dram.total-read-queueing-delay'], results['dram.reads']))
    results['dram.avgqueuewrite'] = map(lambda (a,b): a/(b or 1), zip(results['dram.total-write-queueing-delay'], results['dram.writes']))
    results['dram.totalqueuedelay'] = map(sum, zip(results['dram.total-read-queueing-delay'], results['dram.total-write-queueing-delay']))
    template.append(('  average dram read queueing delay (ns)', 'dram.avgqueueread', format_ns(2)))
    template.append(('  average dram write queueing delay (ns)', 'dram.avgqueuewrite', format_ns(2)))
    template.append(('  total dram queueing delay (ns)', 'dram.avgqueuedelay', format_ns(2)))
  else:
    #print("=== DRAM TOTAL_READ_Q_DELAY: %s" % results['dram.total-queueing-delay'])
    results['dram.avgqueue'] = map(lambda (a,b): a/(b or 1) if b else float('inf'), zip(results.get('dram.total-queueing-delay', [0]*ncores), results['dram.accesses']))
    template.append(('  average dram queueing delay (ns)', 'dram.avgqueue', format_ns(2)))
    template.append(('  total dram queueing delay (ms)', 'dram.total-queueing-delay', format_ms(2)))
  if 'dram-queue.total-time-used' in results:
    results['dram.bandwidth'] = map(lambda a: 100*a/time0 if time0 else float('inf'), results['dram-queue.total-time-used'])
    template.append(('  average dram bandwidth utilization', 'dram.bandwidth', lambda v: '%.2f%%' % v))

  if 'L1-D.loads-where-dram-local' in results:
    results['L1-D.loads-where-dram'] = map(sum, zip(results['L1-D.loads-where-dram-local'], results['L1-D.loads-where-dram-remote']))
    results['L1-D.stores-where-dram'] = map(sum, zip(results['L1-D.stores-where-dram-local'], results['L1-D.stores-where-dram-remote']))
    template.extend([
        ('Coherency Traffic', '', ''),
        ('  num loads from dram', 'L1-D.loads-where-dram' , str),
        ('  num stores from dram', 'L1-D.stores-where-dram' , str),
        ('  num loads from remote dram', 'L1-D.loads-where-dram-remote' , str),
        ('  num stores from remote dram', 'L1-D.stores-where-dram-remote' , str),
        ('  num loads from dram cache', 'L1-D.loads-where-dram-cache' , str),
        #('  num stores from dram cache', 'L1-D.stores-where-dram-cache' , str),
        ('  num loads from remote cache', 'L1-D.loads-where-cache-remote' , str),
        #('  num stores from remote cache', 'L1-D.stores-where-cache-remote' , str),
      ])

  if 'network.shmem-1.bus.num-packets' in results:
      template.extend([
        ('Network summary', '', ''),
        ('  num-packets', 'network.shmem-1.bus.num-packets' , str),
        ('  num-packets-delayed', 'network.shmem-1.bus.num-packets-delayed' , str),
        ('  total-delay (ns)', 'network.shmem-1.bus.total-delay' , format_ns(2)),
        ('  time-used (ns)', 'network.shmem-1.bus.time-used' , format_ns(2)),
      ])
  
  results['performance_model.elapsed_time_max'] = [
    max(results['performance_model.elapsed_time']) if c == 0 else float('inf')
    for c in range(ncores)
  ]
  results['performance_model.ipc_avg'] = [
    sum(results['performance_model.ipc'])/ncores if c == 0 else float('inf')
    for c in range(ncores)
  ]
  results['walltime_total'] = [
    results['walltime'] if c == 0 else float('inf')
    for c in range(ncores)
  ]
  #print(results['walltime'])

  if 'performance_model.elapsed_time' in results:
      template.extend([
        ('Execution time summary', '', ''),
        ('  average IPC', 'performance_model.ipc_avg' , format_float(2)),
        ('  elapsed time (ms)', 'performance_model.elapsed_time' , format_ms(0)),
        ('  max elapsed time (ms)', 'performance_model.elapsed_time_max' , format_ms(0)),
        ('  simulation time (s)', 'walltime_total', format_float(1)),
     ])

  #print(results.keys())

  lines = []
  lines.append([''] + [ 'Core %u' % i for i in range(ncores) ])

  for title, name, func in template:
    line = [ title ]
    if name and name in results:
      for core in range(ncores):
        line.append(' '+func(results[name][core]))
    else:
      line += [''] * ncores
    lines.append(line)


  widths = [ max(10, max([ len(l[i]) for l in lines ])) for i in range(len(lines[0])) ]
  for j, line in enumerate(lines):
    output.write(' | '.join([ ('%%%s%us' % ((j==0 or i==0) and '-' or '', widths[i])) % line[i] for i in range(len(line)) ]) + '\n')



if __name__ == '__main__':
  def usage():
    print 'Usage:', sys.argv[0], '[-h (help)] [--partial <section-start>:<section-end> (default: roi-begin:roi-end)] [-d <resultsdir (default: .)>]'

  jobid = 0
  resultsdir = '.'
  partial = None

  try:
    opts, args = getopt.getopt(sys.argv[1:], "hj:d:", [ 'partial=' ])
  except getopt.GetoptError, e:
    print e
    usage()
    sys.exit()
  for o, a in opts:
    if o == '-h':
      usage()
      sys.exit()
    if o == '-d':
      resultsdir = a
    if o == '-j':
      jobid = long(a)
    if o == '--partial':
      if ':' not in a:
        sys.stderr.write('--partial=<from>:<to>\n')
        usage()
      partial = a.split(':')

  if args:
    usage()
    sys.exit(-1)

  generate_simout(jobid = jobid, resultsdir = resultsdir, partial = partial)
