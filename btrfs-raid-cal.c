#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define u64 unsigned long long

#define APPLY_FIX 0
#define LOG_EXTRA 0

struct btrfs_raid_attr {
	int sub_stripes;	/* sub_stripes info for map */
	int dev_stripes;	/* stripes per dev */
	int devs_max;		/* max devs to use */
	int devs_min;		/* min devs needed */
	int tolerated_failures; /* max tolerated fail devs */
	int devs_increment;	/* ndevs has to be a multiple of this */
	int ncopies;		/* how many copies to data has */
};

enum btrfs_raid_types {
        BTRFS_RAID_RAID10,
        BTRFS_RAID_RAID1,
        BTRFS_RAID_DUP,
        BTRFS_RAID_RAID0,
        BTRFS_RAID_SINGLE,
        BTRFS_RAID_RAID5,
        BTRFS_RAID_RAID6,
        BTRFS_NR_RAID_TYPES
};

const struct btrfs_raid_attr btrfs_raid_array[BTRFS_NR_RAID_TYPES] = {
	[BTRFS_RAID_RAID10] = {
		.sub_stripes	= 2,
		.dev_stripes	= 1,
		.devs_max	= 0,	/* 0 == as many as possible */
#if APPLY_FIX
		.devs_min	= 3,
#else
		.devs_min	= 4,
#endif
		.tolerated_failures = 1,
		.devs_increment	= 2,
		.ncopies	= 2,
	},
	[BTRFS_RAID_RAID1] = {
		.sub_stripes	= 1,
		.dev_stripes	= 1,
		.devs_max	= 2,
#if APPLY_FIX
		.devs_min	= 1,
#else
		.devs_min	= 2,
#endif
		.tolerated_failures = 1,
		.devs_increment	= 2,
		.ncopies	= 2,
	},
	[BTRFS_RAID_DUP] = {
		.sub_stripes	= 1,
		.dev_stripes	= 2,
		.devs_max	= 1,
		.devs_min	= 1,
		.tolerated_failures = 0,
		.devs_increment	= 1,
		.ncopies	= 2,
	},
	[BTRFS_RAID_RAID0] = {
		.sub_stripes	= 1,
		.dev_stripes	= 1,
		.devs_max	= 0,
		.devs_min	= 2,
		.tolerated_failures = 0,
		.devs_increment	= 1,
		.ncopies	= 1,
	},
	[BTRFS_RAID_SINGLE] = {
		.sub_stripes	= 1,
		.dev_stripes	= 1,
		.devs_max	= 1,
		.devs_min	= 1,
		.tolerated_failures = 0,
		.devs_increment	= 1,
		.ncopies	= 1,
	},
	[BTRFS_RAID_RAID5] = {
		.sub_stripes	= 1,
		.dev_stripes	= 1,
		.devs_max	= 0,
		.devs_min	= 2,
		.tolerated_failures = 1,
		.devs_increment	= 1,
		.ncopies	= 2,
	},
	[BTRFS_RAID_RAID6] = {
		.sub_stripes	= 1,
		.dev_stripes	= 1,
		.devs_max	= 0,
		.devs_min	= 3,
		.tolerated_failures = 2,
		.devs_increment	= 1,
		.ncopies	= 3,
	},
};

static const char *btrfs_raid_type_names[BTRFS_NR_RAID_TYPES] = {
        [BTRFS_RAID_RAID10]     = "raid10",
        [BTRFS_RAID_RAID1]      = "raid1",
        [BTRFS_RAID_DUP]        = "dup",
        [BTRFS_RAID_RAID0]      = "raid0",
        [BTRFS_RAID_SINGLE]     = "single",
        [BTRFS_RAID_RAID5]      = "raid5",
        [BTRFS_RAID_RAID6]      = "raid6",
};


void __btrfs_alloc_chunk(int index, int ndevs)
{
	int i, j, s;
	int sub_stripes    = btrfs_raid_array[index].sub_stripes;
	int dev_stripes    = btrfs_raid_array[index].dev_stripes;
	int devs_max       = btrfs_raid_array[index].devs_max;
	int devs_min       = btrfs_raid_array[index].devs_min;
	int devs_increment = btrfs_raid_array[index].devs_increment;
	int ncopies        = btrfs_raid_array[index].ncopies;
	int tolerated_failures = btrfs_raid_array[index].tolerated_failures;
	u64 num_bytes;
	u64 stripe_size;
	int data_stripes;
	int devs_stripes;
	int num_stripes;
	int missing_mirror_dev = 0;
	int x;
	int _ndevs = ndevs;

#if LOG_EXTRA
	printf("\n\"%s\" ndevs %d: sub_stripes %d dev_stripes %d devs_max %d "\
		"devs_min %d tolerated_failures %d devs_increment %d ncopies %d\n",
		btrfs_raid_type_names[index], ndevs, sub_stripes,
		dev_stripes, devs_max, devs_min, tolerated_failures,
		devs_increment, ncopies);
#endif

#if APPLY_FIX
	if (ndevs > (devs_increment * sub_stripes)) {
		ndevs -= ndevs % devs_increment;
	}
#else
	ndevs -= ndevs % devs_increment;
#endif

 	if (ndevs < devs_min) {
	//	printf("RAID FAILED1: ndevs %d < devs_min %d\n",
	//			ndevs, devs_min);
		return;
	}

	if (ndevs < (devs_increment * sub_stripes)) {
#if APPLY_FIX
		missing_mirror_dev = (devs_increment * sub_stripes) - ndevs;
#else
	//	printf("RAID FAILED2: ndevs %d < devs_increment * sub_stripes %d\n",
	//			ndevs, devs_increment * sub_stripes);
		return;
#endif
	}

        if (devs_max && ndevs > devs_max)
		ndevs = devs_max;

	stripe_size = 1;
	num_stripes = ndevs * dev_stripes;
	data_stripes = num_stripes / ncopies;

	switch(index) {
	case BTRFS_RAID_RAID1:
		if (missing_mirror_dev)
			data_stripes = 1;
		break;
	case BTRFS_RAID_RAID5:
		data_stripes = num_stripes - 1;
		break;
	case BTRFS_RAID_RAID6:
		data_stripes = num_stripes - 2;
		break;
	default:
		break;
	}

	for (i = 0; i < ndevs; ++i) {
		for (j = 0; j < dev_stripes; ++j) {
			int s = i * dev_stripes + j;
#if LOG_EXTRA
			printf("dev %d dev_stripes %d s %d (phy_offset = dev_stripes x stripe_size) %llu\n",
				i, j, s, j * stripe_size);
#endif
		}
	}

	num_bytes = stripe_size * data_stripes;

	printf("%s\t_ndevs %d: ndevs %d num_stripes %d data_stripes %d num_bytes %d missing_mirror_dev %d\n",
		btrfs_raid_type_names[index], _ndevs,
		ndevs, num_stripes, data_stripes, num_bytes, missing_mirror_dev);
}

int main()
{
	int i, j;

	for (i = 0; i < BTRFS_NR_RAID_TYPES; i++) {
		for (j = 1; j < 9; j++)
			__btrfs_alloc_chunk(i, j);
		printf("\n");
	}

	return 0;
}
