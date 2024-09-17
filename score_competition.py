import csv
import sqlite3
import sys


usage = """Processes a CSV file with columns Submission,Schedule,Average to determine a score for the COSC240 scheduling competition.

Usage: python3 {} FILE [SCORE]
Where:
  FILE is a CSV that contains Submission,Schedule,Average
  SCORE is the maximum score for the competition (defaults to 100)
"""


def main(file, score = 100):
    """Processes the given CSV file to determine a score for each Submission"""

    # Create database in memory
    conn = sqlite3.connect(':memory:')
    c = conn.cursor()
    c.execute('CREATE TABLE results (submission text, schedule text, average real)')
    c.execute('CREATE TABLE ranks (submission text, schedule text, rank integer, max_rank integer)')
    c.execute('CREATE TABLE max_ranks (schedule text, max_rank integer)')
    c.execute('CREATE TABLE scores (submission text, schedule text, score real)')

    # Insert CSV rows into database
    with open(file) as csv_file:
        reader = csv.DictReader(csv_file)
        for row in reader:
            submission = row['Submission']
            schedule = row['Schedule']
            average = row['Average']
            # Ensure any invalid floats are NULL
            try:
                float(average)
            except ValueError:
                average = 'NULL'
            c.execute('INSERT INTO results VALUES (?,?,?)', (submission, schedule, average))

    # Determine the maximum value of average
    max_val = 0
    for row in c.execute('SELECT average FROM results'):
        if row[0] != 'NULL' and float(row[0]) > max_val:
            max_val = float(row[0])

    # Update all NULL values to have the worst average
    c.execute('UPDATE results SET average=? WHERE average IS NULL OR average = "NULL"', (max_val + 1,))

    # Rank all submissions by average for each schedule
    c.execute('INSERT INTO ranks SELECT submission, schedule, RANK() OVER (PARTITION BY schedule ORDER BY average), 0 FROM results')

    # Calculate maximum ranks for each schedule
    c.execute('INSERT INTO max_ranks SELECT schedule, MAX(rank) FROM ranks GROUP BY schedule')
    c.execute('UPDATE ranks SET max_rank = (SELECT max_rank FROM max_ranks WHERE ranks.schedule=max_ranks.schedule)')

    # Calculate scores
    c.execute('INSERT INTO scores SELECT submission, schedule, (1-(rank-1)/(max_rank*1.0)) AS score FROM ranks')
    c.execute('UPDATE scores SET score = 0 WHERE (submission, schedule) IN (SELECT submission, schedule FROM results WHERE average=?)', (max_val + 1,))

    # Output scores
    for row in c.execute(f'SELECT submission, round(avg(score)*{score},1) as mark from scores GROUP BY submission'):
        print(row[0],row[1])


if __name__ == "__main__":
    # Process arguments
    if not (2 <= len(sys.argv) <= 3):
        print(usage.format(sys.argv[0]))
        sys.exit(1)

    file = sys.argv[1]
    score = 100 if len(sys.argv) <= 2 else int(sys.argv[2])
    main(file, score)
